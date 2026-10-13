#include "poolrad/replay/slot_a_ecl_live.h"
#include "poolrad/recovered/overlay_u1c_and_or_0def.h"

#include <stdio.h>
#include <string.h>

#define RAM_SIZE 655360u
#define DS 0x14A0u
#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #x); \
    return 1; \
} } while (0)

static uint8_t ram[RAM_SIZE];
static pr_rec_ecl_loader_state st;

static size_t p(unsigned seg, unsigned off)
{
    return ((size_t)seg << 4) + off;
}

static void put_word(unsigned seg, unsigned off, uint16_t value)
{
    ram[p(seg, off)] = (uint8_t)value;
    ram[p(seg, off) + 1u] = (uint8_t)(value >> 8);
}

static uint16_t get_word(unsigned seg, unsigned off)
{
    return (uint16_t)((uint16_t)ram[p(seg, off)] |
                      ((uint16_t)ram[p(seg, off) + 1u] << 8));
}

static void put_far(unsigned slot, uint16_t seg, uint16_t off)
{
    put_word(DS, slot, off);
    put_word(DS, slot + 2u, seg);
}

static pr_rec_ecl_handler_status hook(pr_rec_ecl_loader_state *state, void *ctx)
{
    (void)state;
    (void)ctx;
    return PR_REC_ECL_HANDLER_HOOK;
}

static pr_rec_ecl_handler_status stop(pr_rec_ecl_loader_state *state, void *ctx)
{
    (void)ctx;
    state->pc++;
    state->b442e = 1u;
    return PR_REC_ECL_HANDLER_OK;
}

typedef struct {
    uint16_t pc[12];
    unsigned count;
} route_log;

static pr_rec_ecl_handler_status route(pr_rec_ecl_loader_state *state, void *ctx)
{
    route_log *log = ctx;
    if (log->count >= sizeof log->pc / sizeof log->pc[0])
        return PR_REC_ECL_HANDLER_BOUNDS;
    log->pc[log->count++] = state->pc;
    switch (state->latch) {
    case 0x09u: return pr_rec_ecl_op_save(state, NULL);
    case 0x03u: return pr_rec_ecl_op_compare(state);
    case 0x16u: case 0x17u: case 0x18u:
    case 0x19u: case 0x1Au: case 0x1Bu:
        return pr_rec_ecl_op_if(state);
    case 0x01u: return pr_rec_ecl_op_goto(state);
    case 0x2Fu: return pr_u1c_op_and_or(state, NULL, NULL);
    default: return PR_REC_ECL_HANDLER_UNRESOLVED;
    }
}

static int test_synthetic(void)
{
    pr_rec_ecl_dispatch_table table = {0};
    pr_rec_ecl_handler_status hs;
    unsigned steps;
    memset(ram, 0, sizeof ram);
    put_far(0x49D2u, 0x3996u, 0u);
    put_far(0x49D6u, 0x3A16u, 0u);
    put_far(0x49DAu, 0x3A96u, 0u);
    put_far(0x49DEu, 0x3AD6u, 0u);
    put_far(0x5D92u, 0x449Au, 8u);
    ram[p(0x449Au, 8u)] = 3u;
    memcpy(ram + p(0x449Au, 9u), "ABC", 3u);
    put_word(DS, 0x49E3u, 0x9914u);
    put_word(DS, 0x49EDu, 0x9AA5u); /* stale PC before the call */
    put_word(0x3996u, 0x18Eu, 0x1234u);
    put_word(0x3A16u, 0x594u, 1u);
    put_word(0x3A96u, 0x21Eu, 0x5678u);
    ram[p(0x3AD6u, 0x14u)] = 0x09u;
    ram[p(DS, 0x6F7Eu)] = 0x12u;
    ram[p(DS, 0x6F7Fu)] = 0x03u;
    ram[p(DS, 0x6F78u)] = 1u;
    ram[p(DS, 0x84E2u)] = 7u;
    CHECK(pr_slot_a_ecl_bind_ram(NULL, RAM_SIZE, DS, 0x9914u, &st) == PR_SLOT_A_ECL_BIND_ARGUMENT);
    CHECK(pr_slot_a_ecl_bind_ram(ram, RAM_SIZE, DS, 0x9914u, NULL) == PR_SLOT_A_ECL_BIND_ARGUMENT);
    CHECK(pr_slot_a_ecl_bind_ram(ram, RAM_SIZE, DS, 0x9914u, &st) == PR_SLOT_A_ECL_BIND_OK);
    CHECK(st.pc == 0x9914u && get_word(DS, 0x49EDu) == 0x9AA5u);
    CHECK(st.script[0x14u] == 0x09u && st.script_size == 0x1E00u);
    CHECK(st.window0[0x18Eu / 2u] == 0x1234u);
    CHECK(st.window1[0x594u / 2u] == 1u && st.window2[0x21Eu / 2u] == 0x5678u);
    CHECK(st.active_record == ram + p(0x449Au, 8u));
    CHECK(st.active_record_size == 285u && st.active_name_valid == 1u);
    CHECK(st.active_name_len == 3u && memcmp(st.active_name, "ABC", 3u) == 0);
    CHECK(st.entry_addr[0] == 0x9914u && st.cmp_flags[0] == 1u);
    CHECK(st.latch == 0x03u && st.prev_opcode == 0x12u && st.b84e2 == 7u);

    CHECK(pr_rec_ecl_run(&st, &table, 0x9914u, 2u, &steps, &hs) == PR_REC_ECL_RUN_UNMODELLED);
    CHECK(steps == 1u && hs == PR_REC_ECL_HANDLER_OK && st.pc == 0x9914u);
    CHECK(st.latch == 0x09u && st.prev_opcode == 0x03u);
    table.handler[0x09u] = hook;
    CHECK(pr_rec_ecl_run(&st, &table, 0x9914u, 2u, &steps, &hs) == PR_REC_ECL_RUN_HANDLER);
    CHECK(steps == 1u && hs == PR_REC_ECL_HANDLER_HOOK);
    table.handler[0x09u] = stop;
    CHECK(pr_rec_ecl_run(&st, &table, 0x9914u, 2u, &steps, &hs) == PR_REC_ECL_RUN_OK);
    CHECK(steps == 1u && hs == PR_REC_ECL_HANDLER_OK && st.pc == 0x9915u && st.b442e == 0u);

    st.pc = 0xEEEEu;
    put_far(0x49DEu, 0x3AD6u, 0xF000u);
    CHECK(pr_slot_a_ecl_bind_ram(ram, RAM_SIZE, DS, 0x9914u, &st) == PR_SLOT_A_ECL_BIND_BOUNDS);
    CHECK(st.pc == 0xEEEEu);
    put_far(0x49DEu, 0x3AD6u, 0u);
    put_word(DS, 0x6F70u, 1u);
    CHECK(pr_slot_a_ecl_bind_ram(ram, RAM_SIZE, DS, 0x9914u, &st) == PR_SLOT_A_ECL_BIND_CALL_LIST);
    CHECK(st.pc == 0xEEEEu);
    put_word(DS, 0x6F70u, 0u);
    ram[p(0x449Au, 8u)] = 16u;
    CHECK(pr_slot_a_ecl_bind_ram(ram, RAM_SIZE, DS, 0x9914u, &st) == PR_SLOT_A_ECL_BIND_BAD_NAME);
    CHECK(st.pc == 0xEEEEu);
    return 0;
}

static int test_native(const char *path)
{
    FILE *f = fopen(path, "rb");
    pr_rec_ecl_dispatch_table table = {0};
    route_log log = {0};
    static const uint16_t expected_pc[9] = {
        0x9914u, 0x991Au, 0x9920u, 0x9927u, 0x992Eu,
        0x9934u, 0x993Au, 0x993Bu, 0x996Bu
    };
    pr_rec_ecl_handler_status hs;
    unsigned steps, i;
    CHECK(f != NULL);
    CHECK(fread(ram, 1u, RAM_SIZE, f) == RAM_SIZE);
    CHECK(fgetc(f) == EOF && fclose(f) == 0);
    CHECK(get_word(0x1D16u, 0x3EF0u) == 0x345Bu);
    CHECK(get_word(0x1D16u, 0x3EF2u) == 0x3582u);
    CHECK(get_word(0x1D16u, 0x3EF4u) == 0x9914u);
    CHECK(get_word(DS, 0x49EDu) == 0x9AA5u);
    CHECK(pr_slot_a_ecl_bind_ram(ram, RAM_SIZE, DS, 0x9914u, &st) == PR_SLOT_A_ECL_BIND_OK);
    CHECK(st.pc == 0x9914u && st.script[0x14u] == 0x09u);
    CHECK(st.window0[0x1E4u / 2u] == 0x14u && st.window1[0x594u / 2u] == 0u);
    CHECK(st.active_record == ram + p(0x449Au, 8u));
    CHECK(st.active_record_size == 285u && st.active_name_len == 14u);
    CHECK(st.b49ff == 0u && st.entry_addr[0] == 0x9914u);
    CHECK(pr_rec_ecl_run(&st, &table, 0x9914u, 2u, &steps, &hs) == PR_REC_ECL_RUN_UNMODELLED);
    CHECK(steps == 1u && hs == PR_REC_ECL_HANDLER_OK && st.pc == 0x9914u);
    CHECK(st.latch == 0x09u && st.prev_opcode == 0x00u);

    CHECK(pr_slot_a_ecl_bind_ram(ram, RAM_SIZE, DS, 0x9914u, &st) == PR_SLOT_A_ECL_BIND_OK);
    table.ctx = &log;
    table.handler[0x09u] = route;
    table.handler[0x03u] = route;
    table.handler[0x01u] = route;
    table.handler[0x2Fu] = route;
    for (i = 0x16u; i <= 0x1Bu; ++i) table.handler[i] = route;
    CHECK(pr_rec_ecl_run(&st, &table, 0x9914u, 20u, &steps, &hs) == PR_REC_ECL_RUN_UNMODELLED);
    CHECK(steps == 10u && hs == PR_REC_ECL_HANDLER_OK);
    CHECK(log.count == 9u && st.pc == 0x9974u && st.latch == 0x00u);
    for (i = 0u; i < log.count; ++i) CHECK(log.pc[i] == expected_pc[i]);
    CHECK(st.window1[0x37Du] == 0x000Bu && st.b84e8 == 1u);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 1 && argc != 2) return 2;
    if (test_synthetic()) return 1;
    if (argc == 2 && test_native(argv[1])) return 1;
    puts("slot-A live ECL runner state boundary passed");
    return 0;
}
