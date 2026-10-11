#include "poolrad/recovered/resident_dax_selector_02eb.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; \
} } while (0)

typedef struct {
    char events[6];
    unsigned calls;
    unsigned fail_at;
    uint16_t result;
    uint16_t reset_size;
    uint16_t global_offset;
    uint16_t global_value;
    uint8_t copied_length;
    uint8_t copied_first;
    uint8_t copied_last;
    pr_rec_dax_selector_02eb_string *source_to_mutate;
} fixture;

static int event(fixture *f, char code)
{
    f->events[f->calls++] = code;
    f->events[f->calls] = '\0';
    return f->calls == f->fail_at;
}

static int assign_temp(void *context,
                       const pr_rec_dax_selector_02eb_string *name)
{
    fixture *f = context;
    f->copied_length = name->length;
    if (name->length) {
        f->copied_first = name->bytes[0];
        f->copied_last = name->bytes[name->length - 1u];
    }
    if (f->source_to_mutate)
        f->source_to_mutate->length = 0;
    return event(f, 'A');
}

static int reset_temp(void *context, uint16_t record_size)
{
    fixture *f = context;
    f->reset_size = record_size;
    return event(f, 'R');
}

static int close_temp(void *context)
{
    return event(context, 'C');
}

static int io_result(void *context, uint16_t *result)
{
    fixture *f = context;
    if (event(f, 'I')) return 1;
    *result = f->result;
    return 0;
}

static int store_ds_word(void *context, uint16_t offset, uint16_t value)
{
    fixture *f = context;
    f->global_offset = offset;
    f->global_value = value;
    return event(f, 'S');
}

static pr_rec_dax_selector_02eb_io callbacks(fixture *f)
{
    pr_rec_dax_selector_02eb_io io = {
        f, assign_temp, reset_temp, close_temp, io_result, store_ds_word
    };
    return io;
}

static pr_rec_dax_selector_02eb_string synthetic_name(unsigned length)
{
    pr_rec_dax_selector_02eb_string name = {0};
    unsigned i;
    name.length = (uint8_t)length;
    for (i = 0; i < length; ++i) name.bytes[i] = (uint8_t)(i + 1u);
    return name;
}

static int test_completed_paths(void)
{
    fixture f = {0};
    pr_rec_dax_selector_02eb_io io = callbacks(&f);
    pr_rec_dax_selector_02eb_string name = synthetic_name(50);
    uint16_t ax = 0x7777;

    f.source_to_mutate = &name;
    CHECK(pr_rec_res_0802_02eb(&io, &name, &ax) ==
          PR_REC_DAX_SELECTOR_02EB_OK);
    CHECK(ax == 1u && strcmp(f.events, "ARCIS") == 0);
    CHECK(name.length == 0u && f.copied_length == 40u &&
          f.copied_first == 1u && f.copied_last == 40u);
    CHECK(f.reset_size == 0x80u && f.global_offset == 0x49ceu &&
          f.global_value == 0u);

    memset(&f, 0, sizeof f);
    name = synthetic_name(0);
    ax = 0x7777;
    CHECK(pr_rec_res_0802_02eb(&io, &name, &ax) ==
          PR_REC_DAX_SELECTOR_02EB_OK);
    CHECK(ax == 0u && strcmp(f.events, "ARCIS") == 0 &&
          f.copied_length == 0u);

    memset(&f, 0, sizeof f);
    f.result = 0x0100u;
    name = synthetic_name(2);
    CHECK(pr_rec_res_0802_02eb(&io, &name, &ax) ==
          PR_REC_DAX_SELECTOR_02EB_OK);
    CHECK(ax == 0x0100u && f.global_value == 0x0100u &&
          strcmp(f.events, "ARCIS") == 0);

    memset(&f, 0, sizeof f);
    f.result = 0xabcdu;
    CHECK(pr_rec_res_0802_02eb(&io, &name, &ax) ==
          PR_REC_DAX_SELECTOR_02EB_OK);
    CHECK(ax == 0xab00u && f.global_value == 0xabcdu &&
          strcmp(f.events, "ARCIS") == 0);
    return 0;
}

static int test_boundary_failures(void)
{
    pr_rec_dax_selector_02eb_string name = synthetic_name(3);
    unsigned fail;
    for (fail = 1; fail <= 5; ++fail) {
        fixture f = {0};
        pr_rec_dax_selector_02eb_io io = callbacks(&f);
        uint16_t ax = 0x9876u;
        f.fail_at = fail;
        f.result = 0;
        CHECK(pr_rec_res_0802_02eb(&io, &name, &ax) ==
              PR_REC_DAX_SELECTOR_02EB_BOUNDARY);
        CHECK(ax == 0x9876u && f.calls == fail &&
              strncmp(f.events, "ARCIS", fail) == 0);
    }
    return 0;
}

static int test_invalid_arguments(void)
{
    fixture f = {0};
    pr_rec_dax_selector_02eb_io io = callbacks(&f);
    pr_rec_dax_selector_02eb_string name = synthetic_name(1);
    uint16_t ax = 0x6543u;
    CHECK(pr_rec_res_0802_02eb(NULL, &name, &ax) ==
          PR_REC_DAX_SELECTOR_02EB_ARGUMENT);
    CHECK(pr_rec_res_0802_02eb(&io, NULL, &ax) ==
          PR_REC_DAX_SELECTOR_02EB_ARGUMENT);
    CHECK(pr_rec_res_0802_02eb(&io, &name, NULL) ==
          PR_REC_DAX_SELECTOR_02EB_ARGUMENT);
    io.io_result = NULL;
    CHECK(pr_rec_res_0802_02eb(&io, &name, &ax) ==
          PR_REC_DAX_SELECTOR_02EB_ARGUMENT);
    CHECK(ax == 0x6543u && f.calls == 0u);
    return 0;
}

int main(void)
{
    if (test_completed_paths() || test_boundary_failures() ||
        test_invalid_arguments()) return 1;
    return 0;
}
