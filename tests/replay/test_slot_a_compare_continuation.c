/* Bounded COMPARE/IF continuation. The optional private save is hash-gated
 * by the runner script; no original script bytes appear in this test. */
#include "poolrad/formats/dos_savgam.h"
#include "poolrad/recovered/overlay_u1c_random_0234.h"
#include "poolrad/replay/slot_a_native_save.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

typedef struct {
    uint16_t pc, next_pc;
    uint8_t previous, opcode, latch_after;
    uint8_t flags_before[6], flags_after[6];
    uint16_t windows_before[6], windows_after[6];
} route_step;

typedef struct {
    route_step step[6];
    unsigned count;
} route_log;

static void measured_windows(const pr_rec_ecl_loader_state *state, uint16_t words[6]) {
    words[0] = state->window0[PR_SLOT_A_CLOCK_MINUTE_OFFSET / 2u];
    words[1] = state->window0[PR_SLOT_A_CLOCK_TENS_OFFSET / 2u];
    words[2] = state->window0[PR_SLOT_A_CLOCK_HOUR_OFFSET / 2u];
    words[3] = state->window1[0x2CAu];
    words[4] = state->window2[0x10Fu];
    words[5] = state->window0[0x10Bu];
}

static pr_rec_ecl_handler_status record_step(pr_rec_ecl_loader_state *state, void *ctx,
                                              int compare) {
    route_log *log = ctx;
    route_step *step;
    pr_rec_ecl_handler_status status;
    if (log->count >= sizeof log->step / sizeof log->step[0])
        return PR_REC_ECL_HANDLER_BOUNDS;
    step = &log->step[log->count++];
    step->pc = state->pc;
    step->previous = state->prev_opcode;
    step->opcode = state->latch;
    memcpy(step->flags_before, state->cmp_flags, sizeof step->flags_before);
    measured_windows(state, step->windows_before);
    status = compare ? pr_rec_ecl_op_compare(state) : pr_rec_ecl_op_if(state);
    step->next_pc = state->pc;
    step->latch_after = state->latch;
    memcpy(step->flags_after, state->cmp_flags, sizeof step->flags_after);
    measured_windows(state, step->windows_after);
    return status;
}

static pr_rec_ecl_handler_status compare_callback(pr_rec_ecl_loader_state *state, void *ctx) {
    return record_step(state, ctx, 1);
}

static pr_rec_ecl_handler_status if_callback(pr_rec_ecl_loader_state *state, void *ctx) {
    return record_step(state, ctx, 0);
}

static pr_rec_ecl_run_status run_to_exit_entry(pr_rec_ecl_loader_state *state, uint16_t entry,
                                                route_log *log, unsigned max_steps,
                                                unsigned *steps) {
    pr_rec_ecl_dispatch_table table = {0};
    table.ctx = log;
    table.handler[0x03u] = compare_callback;
    table.handler[0x16u] = if_callback;
    table.handler[0x1Au] = if_callback;
    /* EXIT 0x00 intentionally has no callback: stop before its host state. */
    return pr_rec_ecl_run(state, &table, entry, max_steps, steps, NULL);
}

static void map_words(uint16_t *out, const uint8_t *bytes, size_t count) {
    size_t i;
    for (i = 0u; i < count; ++i)
        out[i] = (uint16_t)((uint16_t)bytes[2u * i] | ((uint16_t)bytes[2u * i + 1u] << 8));
}

static void map_save(pr_rec_ecl_loader_state *state, const pr_slot_a_native_state *saved) {
    memset(state, 0, sizeof *state);
    state->script = saved->script;
    state->script_size = sizeof saved->script;
    map_words(state->window0, saved->window0, PR_REC_ECL_WINDOW0_WORDS);
    map_words(state->window1, saved->window1, PR_REC_ECL_WINDOW1_WORDS);
    map_words(state->window2, saved->window2, PR_REC_ECL_WINDOW2_WORDS);
}

static int synthetic_test(void) {
    /* Different PC, destination and thresholds from the saved script.
     * IF = skips a fabricated GOTO; IF <= proceeds to EXIT. */
    static const uint8_t script[] = {
        0x03u, 0x01u, 0x10u, 0x97u, 0x00u, 5u,
        0x16u, 0x01u, 0x00u, 1u,
        0x03u, 0x01u, 0x10u, 0x97u, 0x00u, 5u,
        0x1Au, 0x00u
    };
    static const uint8_t below[6] = {0u, 1u, 1u, 0u, 1u, 0u};
    static const uint16_t pc[4] = {0x9900u, 0x9906u, 0x990Au, 0x9910u};
    static const uint16_t next[4] = {0x9906u, 0x990Au, 0x9910u, 0x9911u};
    pr_rec_ecl_loader_state state = {0};
    route_log log = {0};
    unsigned steps = 0u, i;
    state.script = script;
    state.script_size = sizeof script;
    state.window2[0x10u] = 3u;
    CHECK(run_to_exit_entry(&state, 0x9900u, &log, 5u, &steps) == PR_REC_ECL_RUN_UNMODELLED);
    CHECK(steps == 5u && log.count == 4u);
    for (i = 0u; i < log.count; ++i) {
        CHECK(log.step[i].pc == pc[i] && log.step[i].next_pc == next[i]);
        CHECK(memcmp(log.step[i].flags_after, below, sizeof below) == 0);
    }
    CHECK(log.step[1].latch_after == 0x01u && state.window2[0x10u] == 3u);
    CHECK(state.pc == 0x9911u && state.prev_opcode == 0x1Au && state.latch == 0x00u);
    return 0;
}

static int private_test(const char *path) {
    static pr_slot_a_native_state saved;
    static pr_rec_ecl_loader_state state;
    uint8_t file_data[PR_DOS_SAVGAM_SIZE];
    uint16_t before0[PR_REC_ECL_WINDOW0_WORDS], before1[PR_REC_ECL_WINDOW1_WORDS];
    uint16_t before2[PR_REC_ECL_WINDOW2_WORDS];
    static const uint8_t native_below[6] = {0u, 1u, 1u, 0u, 1u, 0u};
    static const uint16_t native_windows[6] = {1u, 5u, 10u, 0u, 10u, 0u};
    static const uint16_t pc[6] = {0x9B40u, 0x9B46u, 0x9B50u, 0x9B56u, 0x9B60u, 0x9B66u};
    static const uint16_t next[6] = {0x9B46u, 0x9B50u, 0x9B56u, 0x9B60u, 0x9B66u, 0x9B67u};
    static const uint8_t prior[6] = {0x08u, 0x03u, 0x04u, 0x03u, 0x04u, 0x03u};
    static const uint8_t opcode[6] = {0x03u, 0x16u, 0x03u, 0x16u, 0x03u, 0x1Au};
    static const uint8_t latch_after[6] = {0x03u, 0x04u, 0x03u, 0x04u, 0x03u, 0x1Au};
    pr_u1c_random_report random_report;
    route_log log = {0};
    uint32_t seed = UINT32_C(3140872353);
    unsigned steps = 0u, i;
    int trailing;
    FILE *file = fopen(path, "rb");
    CHECK(file != NULL);
    CHECK(fread(file_data, 1u, sizeof file_data, file) == sizeof file_data);
    trailing = fgetc(file);
    CHECK(fclose(file) == 0);
    CHECK(trailing == EOF);
    CHECK(pr_slot_a_native_restore(file_data, sizeof file_data, &saved) == PR_SLOT_A_NATIVE_OK);
    map_save(&state, &saved);
    CHECK(state.window0[0x10Bu] == 0u && state.window1[0x2CAu] == 0u);
    CHECK(state.window2[0x10Fu] == 16u);

    /* PR #232 measured these at the pre-RNG native stop. The clock minute
     * changed since the save; no other unsourced runtime field is needed
     * by COMPARE/IF before EXIT. */
    state.window0[PR_SLOT_A_CLOCK_MINUTE_OFFSET / 2u] = 1u;
    state.b49ff = 0u;
    memcpy(state.cmp_flags, native_below, sizeof native_below);
    state.pc = 0x9B3Au;
    state.latch = 0x08u;
    CHECK(pr_u1c_op_random_seeded(&state, &seed, &random_report) == PR_U1C_RANDOM_OK);
    CHECK(seed == UINT32_C(3238563622) && random_report.argument == 14u);
    CHECK(random_report.dest == 0x980Fu && random_report.result == 10u);
    CHECK(state.pc == 0x9B40u && state.window2[0x10Fu] == 10u);

    memcpy(before0, state.window0, sizeof before0);
    memcpy(before1, state.window1, sizeof before1);
    memcpy(before2, state.window2, sizeof before2);
    CHECK(run_to_exit_entry(&state, 0x9B40u, &log, 7u, &steps) == PR_REC_ECL_RUN_UNMODELLED);
    CHECK(steps == 7u && log.count == 6u);
    for (i = 0u; i < log.count; ++i) {
        CHECK(log.step[i].pc == pc[i] && log.step[i].next_pc == next[i]);
        CHECK(log.step[i].previous == prior[i] && log.step[i].opcode == opcode[i]);
        CHECK(log.step[i].latch_after == latch_after[i]);
        CHECK(memcmp(log.step[i].flags_before, native_below, sizeof native_below) == 0);
        CHECK(memcmp(log.step[i].flags_after, native_below, sizeof native_below) == 0);
        CHECK(memcmp(log.step[i].windows_before, native_windows, sizeof native_windows) == 0);
        CHECK(memcmp(log.step[i].windows_after, native_windows, sizeof native_windows) == 0);
    }
    CHECK(state.pc == 0x9B67u && state.prev_opcode == 0x1Au && state.latch == 0x00u);
    CHECK(state.b442e == 0u && state.b49ff == 0u);
    CHECK(memcmp(state.window0, before0, sizeof before0) == 0);
    CHECK(memcmp(state.window1, before1, sizeof before1) == 0);
    CHECK(memcmp(state.window2, before2, sizeof before2) == 0);
    puts("saved seeded COMPARE/IF: six dispatcher states match; EXIT entry 0x9B67 left unexecuted");
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 2 || synthetic_test() != 0) return 1;
    if (argc == 2 && private_test(argv[1]) != 0) return 1;
    puts("synthetic COMPARE/IF continuation passed");
    return 0;
}
