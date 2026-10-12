#include "poolrad/recovered/overlay_u68_entry2_up.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static uint8_t window1[0x800];
static uint8_t map_buffer[PR_U3BD_FIELD_BUFFER_SIZE];
static uint8_t direction_table[265];

typedef struct {
    char calls[8];
    unsigned count;
    uint8_t key, extended, provide;
    int fail_prepare, fail_input, fail_resident, fail_exit;
    uint8_t resident_selector_update;
    uint16_t source, message;
    uint8_t controls[5], exit_words[4];
    uint8_t resident_word_5aa, resident_x, resident_y;
    uint8_t exit_previous_selector, exit_flag;
} trace;

static void record(trace *t, char call)
{
    if (t->count < sizeof t->calls)
        t->calls[t->count] = call;
    t->count++;
}

static int prepare_prompt(void *ctx, pr_u68_e2_up_state *state,
                          uint16_t source_cs_offset)
{
    trace *t = ctx;
    record(t, 'P');
    t->source = source_cs_offset;
    if (state->preflight.window1[0x592] != 0 ||
        state->preflight.window1[0x593] != 0)
        return 1;
    return t->fail_prepare;
}

static int key_input(void *ctx, pr_u68_e2_up_state *state,
                     uint16_t message_ds_offset, const uint8_t controls[5],
                     pr_u68_e2_key_event *event)
{
    trace *t = ctx;
    (void)state;
    record(t, 'I');
    t->message = message_ds_offset;
    memcpy(t->controls, controls, 5);
    if (t->fail_input)
        return 1;
    if (t->provide) {
        event->key_al = t->key;
        event->extended = t->extended;
        event->provided = 1;
    }
    return 0;
}

static int resident_290b(void *ctx, pr_u68_e2_up_state *state)
{
    trace *t = ctx;
    record(t, 'R');
    t->resident_word_5aa = state->preflight.window1[0x5AA];
    t->resident_x = state->preflight.party_x;
    t->resident_y = state->preflight.party_y;
    if (t->resident_selector_update)
        state->selector_49fa = t->resident_selector_update;
    return t->fail_resident;
}

static int exit_3f1(void *ctx, pr_u68_e2_up_state *state,
                     const uint8_t words_low[4])
{
    trace *t = ctx;
    record(t, 'E');
    memcpy(t->exit_words, words_low, 4);
    t->exit_previous_selector = state->previous_selector_49f9;
    t->exit_flag = state->flag_84e4;
    return t->fail_exit;
}

static void setup(pr_u68_e2_up_state *state, pr_u68_e2_up_hooks *hooks,
                  trace *t)
{
    memset(state, 0, sizeof *state);
    memset(hooks, 0, sizeof *hooks);
    memset(t, 0, sizeof *t);
    memset(window1, 0xA5, sizeof window1);
    memset(map_buffer, 0xFF, sizeof map_buffer);
    memset(direction_table, 0, sizeof direction_table);
    direction_table[6] = 0xFF; /* synthetic facing-6 x step -1 */
    state->selector_49fa = 1;
    state->previous_selector_49f9 = 0xA5;
    state->flag_84e4 = 0;
    state->preflight.party_x = 15;
    state->preflight.party_y = 4;
    state->preflight.facing = 6;
    state->preflight.window1 = window1;
    state->preflight.window1_size = sizeof window1;
    state->preflight.direction_table = direction_table;
    state->preflight.direction_table_size = sizeof direction_table;
    state->preflight.field.map_id = 1;
    state->preflight.field.buffer = map_buffer;
    state->preflight.field.entry2_frame_residue = 0x5A;
    hooks->prepare_prompt = prepare_prompt;
    hooks->key_input = key_input;
    hooks->resident_290b = resident_290b;
    hooks->exit_3f1_0137 = exit_3f1;
    hooks->ctx = t;
    state->hooks = hooks;
    t->key = PR_U68_E2_KEY_UP;
    t->extended = 1;
    t->provide = 1;
}

int main(void)
{
    pr_u68_e2_up_state state;
    pr_u68_e2_up_hooks hooks;
    trace t;
    uint8_t al = 0xEE;
    uint16_t stop = 0;

    /* The complete slice: window clear, preparation, input, merged
     * preflight, resident, selector copy, conditional exit, AL key. */
    setup(&state, &hooks, &t);
    CHECK(pr_u68_entry2_up_slice(&state, &al, &stop) == PR_U68_E2_UP_OK);
    CHECK(stop == 0x0D4Cu && al == PR_U68_E2_KEY_UP);
    CHECK(window1[0x592] == 0 && window1[0x593] == 0);
    CHECK(window1[0x5AA] == 0 && window1[0x5AB] == 0);
    CHECK(t.count == 4 && memcmp(t.calls, "PIRE", 4) == 0);
    CHECK(t.source == 0x97Au && t.message == 0x4B6u);
    CHECK(memcmp(t.controls, (const uint8_t[]){0x0D,0x0A,0x0F,1,0}, 5) == 0);
    CHECK(memcmp(t.exit_words, (const uint8_t[]){1,0x11,0x26,0x16}, 4) == 0);
    CHECK(t.resident_word_5aa == 0 && t.resident_x == 15 && t.resident_y == 4);
    CHECK(t.exit_previous_selector == 1 && t.exit_flag == 0);
    CHECK(state.previous_selector_49f9 == 1 && state.flag_84e4 == 1);
    CHECK(state.preflight.party_x == 15 && state.preflight.party_y == 4);

    /* Already-set exit flag skips only +0x0D35; no exit hook is needed. */
    setup(&state, &hooks, &t);
    state.flag_84e4 = 1;
    hooks.exit_3f1_0137 = NULL;
    CHECK(pr_u68_entry2_up_slice(&state, &al, &stop) == PR_U68_E2_UP_OK);
    CHECK(t.count == 3 && memcmp(t.calls, "PIR", 3) == 0);
    CHECK(state.flag_84e4 == 1 && al == PR_U68_E2_KEY_UP);

    /* The resident may update selector before +0x0D1C reads it. */
    setup(&state, &hooks, &t);
    t.resident_selector_update = 4;
    CHECK(pr_u68_entry2_up_slice(&state, &al, &stop) == PR_U68_E2_UP_OK);
    CHECK(state.previous_selector_49f9 == 4 && t.exit_previous_selector == 4);

    /* Selector and non-Up prompt paths are explicit unsupported slices;
     * no key result or resident/exit effect is fabricated. */
    setup(&state, &hooks, &t);
    state.selector_49fa = 2;
    al = 0xEE;
    CHECK(pr_u68_entry2_up_slice(&state, &al, &stop)
          == PR_U68_E2_UP_OUTSIDE_SLICE && stop == 0x0BACu);
    CHECK(t.count == 0 && al == 0xEE && window1[0x592] == 0);
    CHECK(state.previous_selector_49f9 == 0xA5 && state.flag_84e4 == 0);
    setup(&state, &hooks, &t);
    t.extended = 0;
    CHECK(pr_u68_entry2_up_slice(&state, &al, &stop)
          == PR_U68_E2_UP_OUTSIDE_SLICE && stop == 0x09E1u);
    CHECK(t.count == 2 && memcmp(t.calls, "PI", 2) == 0 && al == 0xEE);
    setup(&state, &hooks, &t);
    t.key = 0x50;
    CHECK(pr_u68_entry2_up_slice(&state, &al, &stop)
          == PR_U68_E2_UP_OUTSIDE_SLICE && stop == 0x0ABCu);
    CHECK(t.count == 2 && al == 0xEE);

    /* Required hooks fail when reached, preserving earlier effects. */
    setup(&state, &hooks, &t);
    hooks.prepare_prompt = NULL;
    CHECK(pr_u68_entry2_up_slice(&state, &al, &stop)
          == PR_U68_E2_UP_HOOK && stop == 0x09B2u);
    CHECK(t.count == 0 && window1[0x592] == 0 && al == 0xEE);
    setup(&state, &hooks, &t);
    hooks.key_input = NULL;
    CHECK(pr_u68_entry2_up_slice(&state, &al, &stop)
          == PR_U68_E2_UP_HOOK && stop == 0x09D0u);
    CHECK(t.count == 1 && t.calls[0] == 'P' && al == 0xEE);
    setup(&state, &hooks, &t);
    t.provide = 0; /* even a zero-returning empty input hook is rejected */
    CHECK(pr_u68_entry2_up_slice(&state, &al, &stop)
          == PR_U68_E2_UP_HOOK && stop == 0x09D0u);
    CHECK(t.count == 2 && al == 0xEE);
    setup(&state, &hooks, &t);
    hooks.resident_290b = NULL;
    CHECK(pr_u68_entry2_up_slice(&state, &al, &stop)
          == PR_U68_E2_UP_HOOK && stop == 0x0B9Bu);
    CHECK(t.count == 2 && window1[0x5AA] == 0 && al == 0xEE);
    CHECK(state.previous_selector_49f9 == 0xA5);
    setup(&state, &hooks, &t);
    hooks.exit_3f1_0137 = NULL;
    CHECK(pr_u68_entry2_up_slice(&state, &al, &stop)
          == PR_U68_E2_UP_HOOK && stop == 0x0D35u);
    CHECK(t.count == 3 && state.previous_selector_49f9 == 1);
    CHECK(state.flag_84e4 == 0 && al == 0xEE);

    setup(&state, &hooks, &t);
    t.fail_prepare = 1;
    CHECK(pr_u68_entry2_up_slice(&state, &al, &stop)
          == PR_U68_E2_UP_HOOK && stop == 0x09B2u);
    CHECK(t.count == 1 && t.calls[0] == 'P' && al == 0xEE);
    setup(&state, &hooks, &t);
    t.fail_input = 1;
    CHECK(pr_u68_entry2_up_slice(&state, &al, &stop)
          == PR_U68_E2_UP_HOOK && stop == 0x09D0u);
    CHECK(t.count == 2 && memcmp(t.calls, "PI", 2) == 0 && al == 0xEE);
    setup(&state, &hooks, &t);
    t.fail_resident = 1;
    CHECK(pr_u68_entry2_up_slice(&state, &al, &stop)
          == PR_U68_E2_UP_HOOK && stop == 0x0B9Bu);
    CHECK(t.count == 3 && state.previous_selector_49f9 == 0xA5 && al == 0xEE);
    setup(&state, &hooks, &t);
    t.fail_exit = 1;
    CHECK(pr_u68_entry2_up_slice(&state, &al, &stop)
          == PR_U68_E2_UP_HOOK && stop == 0x0D35u);
    CHECK(t.count == 4 && state.previous_selector_49f9 == 1);
    CHECK(state.flag_84e4 == 0 && al == 0xEE);

    /* The preflight guard is reached after the prompt; its window-1
     * 0x5AA clear has already occurred, but resident has not run. */
    setup(&state, &hooks, &t);
    state.preflight.field.buffer = NULL;
    CHECK(pr_u68_entry2_up_slice(&state, &al, &stop)
          == PR_U68_E2_UP_PREFLIGHT && stop == 0x0AB2u);
    CHECK(t.count == 2 && window1[0x5AA] == 0 && al == 0xEE);

    /* The first word write is guarded before any memory or hook effect. */
    setup(&state, &hooks, &t);
    state.preflight.window1_size = 0x593;
    CHECK(pr_u68_entry2_up_slice(&state, &al, &stop)
          == PR_U68_E2_UP_ARGUMENT && stop == 0x098Fu);
    CHECK(window1[0x592] == 0xA5 && t.count == 0 && al == 0xEE);
    CHECK(pr_u68_entry2_up_slice(NULL, &al, &stop) == PR_U68_E2_UP_ARGUMENT);
    CHECK(pr_u68_entry2_up_slice(&state, NULL, &stop) == PR_U68_E2_UP_ARGUMENT);

    puts("overlay_u68_entry2_up: ok");
    return 0;
}
