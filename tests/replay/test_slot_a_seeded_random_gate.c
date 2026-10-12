/* One bounded ECL dispatch. Synthetic coverage is public; the optional
 * hash-checked slot-A save stays under ignored resources/. */
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
    uint32_t *seed;
    pr_u1c_random_status status;
    pr_u1c_random_report report;
} random_gate;

static pr_rec_ecl_handler_status random_callback(pr_rec_ecl_loader_state *state, void *ctx) {
    random_gate *gate = ctx;
    gate->status = pr_u1c_op_random_seeded(state, gate->seed, &gate->report);
    if (gate->status == PR_U1C_RANDOM_OK) return PR_REC_ECL_HANDLER_OK;
    if (gate->status == PR_U1C_RANDOM_CORE) return gate->report.core_status;
    return PR_REC_ECL_HANDLER_UNRESOLVED;
}

static void map_words(uint16_t *out, const uint8_t *bytes, size_t count) {
    size_t i;
    for (i = 0; i < count; ++i)
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

static pr_rec_ecl_run_status dispatch_once(pr_rec_ecl_loader_state *state, uint16_t entry,
                                            random_gate *gate, unsigned *steps,
                                            pr_rec_ecl_handler_status *handler_status) {
    pr_rec_ecl_dispatch_table table = {0};
    table.ctx = gate;
    table.handler[0x08u] = random_callback;
    return pr_rec_ecl_run(state, &table, entry, 1u, steps, handler_status);
}

static int synthetic_test(void) {
    /* Different operands and address from the private saved script. */
    static const uint8_t script[] = {0x08u, 0x00u, 4u, 0x01u, 0x10u, 0x97u, 0x03u};
    pr_rec_ecl_loader_state state = {0};
    random_gate gate = {0};
    pr_rec_ecl_handler_status handler_status;
    pr_rec_ecl_run_status run_status;
    uint32_t seed = 0u;
    unsigned steps = 0u;

    state.script = script;
    state.script_size = sizeof script;
    state.window2[0x10u] = 17u;
    gate.seed = &seed;
    run_status = dispatch_once(&state, 0x9900u, &gate, &steps, &handler_status);
    CHECK(run_status == PR_REC_ECL_RUN_STEP_LIMIT && steps == 1u);
    CHECK(handler_status == PR_REC_ECL_HANDLER_OK && gate.status == PR_U1C_RANDOM_OK);
    CHECK(seed == 1u && gate.report.argument == 5u && gate.report.result == 0u);
    CHECK(gate.report.dest == 0x9710u && gate.report.stored && state.window2[0x10u] == 0u);
    CHECK(state.pc == 0x9906u && state.latch == 0x03u && state.prev_opcode == 0x08u);

    memset(&state, 0, sizeof state);
    state.script = script;
    state.script_size = sizeof script;
    state.window2[0x10u] = 17u;
    memset(&gate, 0, sizeof gate);
    run_status = dispatch_once(&state, 0x9900u, &gate, &steps, &handler_status);
    CHECK(run_status == PR_REC_ECL_RUN_HANDLER && steps == 1u);
    CHECK(handler_status == PR_REC_ECL_HANDLER_UNRESOLVED);
    CHECK(gate.status == PR_U1C_RANDOM_SEED_MISSING && state.pc == 0x9906u);
    CHECK(state.window2[0x10u] == 17u && state.latch == 0x08u);
    return 0;
}

static int private_test(const char *path) {
    uint8_t file_data[PR_DOS_SAVGAM_SIZE];
    static pr_slot_a_native_state saved;
    static pr_rec_ecl_loader_state state;
    random_gate gate = {0};
    pr_rec_ecl_handler_status handler_status;
    pr_rec_ecl_run_status run_status;
    uint32_t seed = UINT32_C(3140872353);
    unsigned steps = 0u;
    FILE *file = fopen(path, "rb");
    CHECK(file != NULL);
    CHECK(fread(file_data, 1u, sizeof file_data, file) == sizeof file_data);
    CHECK(fgetc(file) == EOF);
    CHECK(fclose(file) == 0);
    CHECK(pr_slot_a_native_restore(file_data, sizeof file_data, &saved) == PR_SLOT_A_NATIVE_OK);
    map_save(&state, &saved);
    CHECK(state.window2[0x10Fu] == 16u);
    CHECK(state.script[0x23Au] == 0x08u && state.script[0x240u] == 0x03u);
    gate.seed = &seed;
    run_status = dispatch_once(&state, 0x9B3Au, &gate, &steps, &handler_status);
    CHECK(run_status == PR_REC_ECL_RUN_STEP_LIMIT && steps == 1u);
    CHECK(handler_status == PR_REC_ECL_HANDLER_OK && gate.status == PR_U1C_RANDOM_OK);
    CHECK(gate.report.slot_byte == 13u && gate.report.argument == 14u);
    CHECK(gate.report.dest == 0x980Fu && gate.report.result == 10u && gate.report.stored);
    CHECK(seed == UINT32_C(3238563622) && state.window2[0x10Fu] == 10u);
    CHECK(state.pc == 0x9B40u && state.latch == 0x03u && state.prev_opcode == 0x08u);
    printf("saved slot-A local gate: PC=0x%04X next opcode=0x%02X, window2 +0x21E 16->%u, seed=%u\n",
           state.pc, state.latch, state.window2[0x10Fu], seed);
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 2 || synthetic_test() != 0) return 1;
    if (argc == 2 && private_test(argv[1]) != 0) return 1;
    puts("synthetic seeded ECL run gate passed");
    return 0;
}
