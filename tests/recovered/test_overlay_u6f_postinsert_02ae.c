#include "poolrad/recovered/overlay_u6f_postinsert_02ae.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint8_t *live;
    uint16_t words[4];
    size_t next_word;
    size_t make_calls, io_calls, display_calls;
    size_t path_lengths[4], message_lengths[4];
    uint8_t paths[4][128], messages[4][41];
    int fail_make, fail_io, fail_display;
    int mutate_on_first_make;
} fixture;

static int make_directory(void *context, const uint8_t *path, size_t length)
{
    fixture *f = context;
    size_t i = f->make_calls++;
    assert(i < 4u && length <= 127u && path[length] == 0u);
    f->path_lengths[i] = length;
    memcpy(f->paths[i], path, length + 1u);
    if (f->mutate_on_first_make && i == 0u) {
        f->live[0] = 4u;
        memcpy(f->live + 1u, "NEW!", 4u);
    }
    return f->fail_make;
}

static int io_result(void *context, uint16_t *result)
{
    fixture *f = context;
    ++f->io_calls;
    if (f->fail_io)
        return 1;
    assert(f->next_word < 4u);
    *result = f->words[f->next_word++];
    return 0;
}

static int display_and_input(void *context, const uint8_t *message,
                             size_t span, uint8_t color)
{
    fixture *f = context;
    size_t i = f->display_calls++;
    assert(i < 4u && span <= 41u && span == (size_t)message[0] + 1u);
    assert(color == 0x0eu);
    f->message_lengths[i] = span;
    memcpy(f->messages[i], message, span);
    return f->fail_display;
}

static pr_u6f_postinsert_02ae_hooks hooks_for(fixture *f)
{
    pr_u6f_postinsert_02ae_hooks hooks = {
        f, make_directory, io_result, display_and_input
    };
    return hooks;
}

static void test_success_and_five(void)
{
    uint8_t live[] = {5u, 'a', 'b', 'c', 'd', '!'};
    fixture f = {0};
    pr_u6f_postinsert_02ae_hooks hooks = hooks_for(&f);
    pr_u6f_postinsert_02ae_result out;
    pr_u6f_postinsert_02ae_status status;
    f.live = live;

    status = pr_u6f_postinsert_02ae(live, sizeof live, NULL, 0u,
                                    NULL, 0u, &hooks, 2u, &out);
    assert(status == PR_U6F_POSTINSERT_02AE_OK);
    assert(out.next_offset == 0x032eu && out.attempts == 1u &&
           out.displays == 0u && out.has_io_result && out.last_io_result == 0u);
    assert(f.make_calls == 1u && f.io_calls == 1u && f.display_calls == 0u);
    assert(f.path_lengths[0] == 4u && memcmp(f.paths[0], "abcd", 5u) == 0);

    memset(&f, 0, sizeof f);
    f.live = live;
    f.words[0] = 5u;
    hooks = hooks_for(&f);
    hooks.display_and_input = NULL;
    status = pr_u6f_postinsert_02ae(live, sizeof live, NULL, 0u,
                                    NULL, 0u, &hooks, 1u, &out);
    assert(status == PR_U6F_POSTINSERT_02AE_OK);
    assert(out.last_io_result == 5u && out.displays == 0u);
}

static void test_retry_and_live_read(void)
{
    uint8_t live[] = {4u, 'O', 'L', 'D', '!'};
    uint8_t prefix[] = {2u, 'P', ':'};
    uint8_t suffix[] = {1u, '.'};
    fixture f = {0};
    pr_u6f_postinsert_02ae_hooks hooks = hooks_for(&f);
    pr_u6f_postinsert_02ae_result out;
    f.live = live;
    f.words[0] = 3u;
    f.words[1] = 0u;
    f.mutate_on_first_make = 1;
    assert(pr_u6f_postinsert_02ae(live, sizeof live, prefix, sizeof prefix,
                                  suffix, sizeof suffix, &hooks, 2u, &out) ==
           PR_U6F_POSTINSERT_02AE_OK);
    assert(out.attempts == 2u && out.displays == 1u && out.next_offset == 0x032eu);
    assert(f.make_calls == 2u && f.io_calls == 2u && f.display_calls == 1u);
    assert(memcmp(f.paths[0], "OLD", 4u) == 0);
    assert(memcmp(f.paths[1], "NEW", 4u) == 0);
    /* Raw DS:537B is read after the first host call, so mutation matters. */
    assert(f.message_lengths[0] == 5u);
    assert(f.messages[0][0] == 4u &&
           memcmp(f.messages[0] + 1u, "P:N.", 4u) == 0);
}

static void test_retry_guard_and_missing_reached_callbacks(void)
{
    uint8_t live[] = {1u, 'x'};
    uint8_t prefix[] = {1u, 'P'}, suffix[] = {1u, '.'};
    fixture f = {0};
    pr_u6f_postinsert_02ae_hooks hooks = hooks_for(&f);
    pr_u6f_postinsert_02ae_result out;
    f.live = live;
    f.words[0] = 2u;
    f.words[1] = 2u;
    assert(pr_u6f_postinsert_02ae(live, sizeof live, prefix, sizeof prefix,
                                  suffix, sizeof suffix, &hooks, 2u, &out) ==
           PR_U6F_POSTINSERT_02AE_RETRY_LIMIT);
    assert(out.next_offset == 0x02b5u && out.attempts == 2u && out.displays == 2u);
    assert(f.make_calls == 2u && f.io_calls == 2u && f.display_calls == 2u);
    assert(f.path_lengths[0] == 0u && f.paths[0][0] == 0u);

    memset(&f, 0, sizeof f);
    f.live = live;
    hooks = hooks_for(&f);
    hooks.make_directory = NULL;
    assert(pr_u6f_postinsert_02ae(live, sizeof live, NULL, 0u,
                                  NULL, 0u, &hooks, 1u, &out) ==
           PR_U6F_POSTINSERT_02AE_NEED_MKDIR);
    assert(out.next_offset == 0x02d0u && out.attempts == 0u);
    hooks.make_directory = make_directory;
    hooks.io_result = NULL;
    assert(pr_u6f_postinsert_02ae(live, sizeof live, NULL, 0u,
                                  NULL, 0u, &hooks, 1u, &out) ==
           PR_U6F_POSTINSERT_02AE_NEED_IO_RESULT);
    assert(out.next_offset == 0x02d5u && out.attempts == 1u && !out.has_io_result);
    hooks.io_result = io_result;
    f.words[0] = 2u;
    assert(pr_u6f_postinsert_02ae(live, sizeof live, NULL, 0u,
                                  NULL, 0u, &hooks, 1u, &out) ==
           PR_U6F_POSTINSERT_02AE_NEED_MESSAGE);
    assert(out.next_offset == 0x02ecu && out.attempts == 1u && out.has_io_result);
    hooks.display_and_input = NULL;
    f.next_word = 0u;
    assert(pr_u6f_postinsert_02ae(live, sizeof live, prefix, sizeof prefix,
                                  suffix, sizeof suffix, &hooks, 1u, &out) ==
           PR_U6F_POSTINSERT_02AE_NEED_DISPLAY);
    assert(out.next_offset == 0x031au && out.attempts == 1u && out.displays == 0u);
}

static void test_callee_caps_and_bounds(void)
{
    uint8_t live[201] = {0};
    uint8_t prefix[51] = {0};
    uint8_t suffix[] = {1u, '.'};
    fixture f = {0};
    pr_u6f_postinsert_02ae_hooks hooks = hooks_for(&f);
    pr_u6f_postinsert_02ae_result out;
    size_t i;
    f.live = live;
    live[0] = 200u;
    for (i = 1u; i < sizeof live; ++i)
        live[i] = 'q';
    prefix[0] = 50u;
    memset(prefix + 1u, 'p', 50u);
    f.words[0] = 8u;
    assert(pr_u6f_postinsert_02ae(live, sizeof live, prefix, sizeof prefix,
                                  suffix, sizeof suffix, &hooks, 1u, &out) ==
           PR_U6F_POSTINSERT_02AE_RETRY_LIMIT);
    assert(f.path_lengths[0] == 127u && f.paths[0][127] == 0u);
    assert(f.message_lengths[0] == 41u && f.messages[0][0] == 40u);
    for (i = 1u; i < 41u; ++i)
        assert(f.messages[0][i] == 'p');

    memset(&f, 0, sizeof f);
    f.live = live;
    hooks = hooks_for(&f);
    assert(pr_u6f_postinsert_02ae(live, 2u, NULL, 0u,
                                  NULL, 0u, &hooks, 1u, &out) ==
           PR_U6F_POSTINSERT_02AE_BOUNDS);
    assert(f.make_calls == 0u);
    live[0] = 0u;
    f.words[0] = 7u;
    assert(pr_u6f_postinsert_02ae(live, 1u, prefix, sizeof prefix,
                                  suffix, sizeof suffix, &hooks, 1u, &out) ==
           PR_U6F_POSTINSERT_02AE_BOUNDS);
    assert(out.next_offset == 0x02ecu && out.attempts == 1u && out.has_io_result);
}

static void test_host_failure(void)
{
    uint8_t live[] = {2u, 'X', '!'};
    uint8_t prefix[] = {1u, 'P'}, suffix[] = {1u, '.'};
    fixture f = {0};
    pr_u6f_postinsert_02ae_hooks hooks = hooks_for(&f);
    pr_u6f_postinsert_02ae_result out;
    f.live = live;
    f.fail_make = 1;
    assert(pr_u6f_postinsert_02ae(live, sizeof live, prefix, sizeof prefix,
                                  suffix, sizeof suffix, &hooks, 1u, &out) ==
           PR_U6F_POSTINSERT_02AE_HOST_FAILURE);
    assert(out.attempts == 1u && f.io_calls == 0u && out.next_offset == 0x02d0u);
    f.fail_make = 0;
    f.fail_io = 1;
    assert(pr_u6f_postinsert_02ae(live, sizeof live, prefix, sizeof prefix,
                                  suffix, sizeof suffix, &hooks, 1u, &out) ==
           PR_U6F_POSTINSERT_02AE_HOST_FAILURE);
    assert(out.attempts == 1u && out.next_offset == 0x02d5u);
    f.fail_io = 0;
    f.fail_display = 1;
    f.words[0] = 9u;
    assert(pr_u6f_postinsert_02ae(live, sizeof live, prefix, sizeof prefix,
                                  suffix, sizeof suffix, &hooks, 1u, &out) ==
           PR_U6F_POSTINSERT_02AE_HOST_FAILURE);
    assert(out.displays == 1u && out.next_offset == 0x031au);
}

static void test_zero_length_reads_raw_second_byte(void)
{
    uint8_t live[] = {0u, 'z'};
    uint8_t prefix[] = {1u, '['}, suffix[] = {1u, ']'};
    fixture f = {0};
    pr_u6f_postinsert_02ae_hooks hooks = hooks_for(&f);
    pr_u6f_postinsert_02ae_result out;
    f.live = live;
    f.words[0] = 6u;
    assert(pr_u6f_postinsert_02ae(live, sizeof live, prefix, sizeof prefix,
                                  suffix, sizeof suffix, &hooks, 1u, &out) ==
           PR_U6F_POSTINSERT_02AE_RETRY_LIMIT);
    assert(f.path_lengths[0] == 0u && f.paths[0][0] == 0u);
    assert(f.message_lengths[0] == 4u && f.messages[0][0] == 3u);
    assert(memcmp(f.messages[0] + 1u, "[z]", 3u) == 0);
}

int main(void)
{
    test_success_and_five();
    test_retry_and_live_read();
    test_retry_guard_and_missing_reached_callbacks();
    test_callee_caps_and_bounds();
    test_host_failure();
    test_zero_length_reads_raw_second_byte();
    return 0;
}
