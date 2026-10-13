#include "poolrad/replay/slot_a_first_exit.h"

#include <stdio.h>
#include <string.h>

#define RAM_SIZE 655360u
#define DS 0x14A0u
#define SS 0x1D16u
#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #x); \
    return 1; \
} } while (0)

static uint8_t ram[RAM_SIZE], and_ram[RAM_SIZE], search_ram[RAM_SIZE];
static pr_rec_ecl_loader_state state;

static size_t p(unsigned seg, unsigned off)
{
    return ((size_t)seg << 4) + off;
}

static uint16_t word_at(const uint8_t *data, unsigned seg, unsigned off)
{
    size_t at = p(seg, off);
    return (uint16_t)((uint16_t)data[at] | ((uint16_t)data[at + 1u] << 8));
}

static void put_word(unsigned seg, unsigned off, uint16_t v)
{
    ram[p(seg, off)] = (uint8_t)v;
    ram[p(seg, off) + 1u] = (uint8_t)(v >> 8);
}

static void put_far(unsigned slot, uint16_t seg)
{
    put_word(DS, slot, 0u);
    put_word(DS, slot + 2u, seg);
}

static int load_ram(const char *path, uint8_t *buffer)
{
    FILE *f = fopen(path, "rb");
    int complete;
    if (!f) return 0;
    complete = fread(buffer, 1u, RAM_SIZE, f) == RAM_SIZE && fgetc(f) == EOF;
    if (fclose(f) != 0) return 0;
    return complete;
}

static int synthetic(void)
{
    pr_u1c_exit_env env = {0};
    pr_slot_a_first_exit_report report;
    memset(ram, 0, sizeof ram);
    put_far(0x49D2u, 0x3996u);
    put_far(0x49D6u, 0x3A16u);
    put_far(0x49DAu, 0x3A96u);
    put_far(0x49DEu, 0x3AD6u);
    ram[p(0x3AD6u, 0x14u)] = 0x00u; /* EXIT at the saved entry */
    ram[p(DS, 0x84C8u)] = 3u;
    ram[p(DS, 0x84C9u)] = 4u;
    ram[p(DS, 0x84E0u)] = 5u;
    ram[p(DS, 0x5E24u)] = 0u;
    ram[p(DS, 0x5E25u)] = 0u;
    CHECK(pr_slot_a_first_exit_run_ram(ram, RAM_SIZE, DS, 2u,
           &state, &env, &report) == PR_SLOT_A_ECL_BIND_OK);
    CHECK(report.run_status == PR_REC_ECL_RUN_OK && report.handler_status == PR_REC_ECL_HANDLER_OK);
    CHECK(report.steps == 1u && report.callback_count == 1u && report.callback_pc[0] == 0x9914u);
    CHECK(report.exit_reached && report.exit_status == PR_U1C_EXIT_OK);
    CHECK(report.exit_report.boxes == 0u && report.exit_report.freed_nodes == 0u);
    CHECK(state.pc == 0x9915u && state.b442e == 0u);
    CHECK(state.b84c8 == 0u && state.b84c9 == 0u && state.b84e0 == 0u);
    CHECK(state.b5e24 == 1u && state.b5e25 == 0x11u);

    ram[p(DS, 0x84E2u)] = 1u;
    CHECK(pr_slot_a_first_exit_run_ram(ram, RAM_SIZE, DS, 2u,
           &state, &env, &report) == PR_SLOT_A_ECL_BIND_OK);
    CHECK(report.run_status == PR_REC_ECL_RUN_HANDLER && report.handler_status == PR_REC_ECL_HANDLER_HOOK);
    CHECK(report.exit_reached && report.exit_status == PR_U1C_EXIT_HOOK);
    CHECK(report.steps == 1u && state.pc == 0x9914u && state.b84e2 == 1u);
    CHECK(state.b84c8 == 3u && state.b5e24 == 0u);

    ram[p(DS, 0x84E2u)] = 0u;
    ram[p(0x3AD6u, 0x14u)] = 0x0Eu; /* a real case without a bound handler */
    CHECK(pr_slot_a_first_exit_run_ram(ram, RAM_SIZE, DS, 2u,
           &state, &env, &report) == PR_SLOT_A_ECL_BIND_OK);
    CHECK(report.run_status == PR_REC_ECL_RUN_UNMODELLED && !report.exit_reached);
    CHECK(report.steps == 1u && state.pc == 0x9914u);

    state.pc = 0xEEEEu;
    env.b4430 = 0xAAu;
    put_word(DS, 0x6F70u, 1u);
    CHECK(pr_slot_a_first_exit_run_ram(ram, RAM_SIZE, DS, 2u,
           &state, &env, &report) == PR_SLOT_A_ECL_BIND_CALL_LIST);
    CHECK(report.bind_status == PR_SLOT_A_ECL_BIND_CALL_LIST && report.run_status == PR_REC_ECL_RUN_ARGUMENT);
    CHECK(state.pc == 0xEEEEu && env.b4430 == 0xAAu);
    return 0;
}

static int private_route(const char *pre, const char *and_path, const char *search)
{
    static const uint16_t expected_pc[10] = {
        0x9914u, 0x991Au, 0x9920u, 0x9927u, 0x992Eu,
        0x9934u, 0x993Au, 0x993Bu, 0x996Bu, 0x9974u
    };
    pr_u1c_exit_env env = {0};
    pr_slot_a_first_exit_report report;
    unsigned i;
    CHECK(load_ram(pre, ram) && load_ram(and_path, and_ram) && load_ram(search, search_ram));
    CHECK(word_at(ram, SS, 0x3EF0u) == 0x345Bu && word_at(ram, SS, 0x3EF2u) == 0x3582u);
    CHECK(word_at(ram, SS, 0x3EF4u) == 0x9914u);
    CHECK(word_at(and_ram, DS, 0x49EDu) == 0x996Bu);
    CHECK(and_ram[p(DS, 0x6F7Eu)] == 0x01u && and_ram[p(DS, 0x6F7Fu)] == 0x2Fu);
    CHECK(and_ram[p(DS, 0x84E2u)] == 0u && and_ram[p(DS, 0x4430u)] == 0u);
    CHECK(word_at(and_ram, DS, 0x6F70u) == 0u && word_at(and_ram, DS, 0x6F72u) == 0u);
    CHECK(word_at(search_ram, SS, 0x3EF0u) == 0x34EAu);
    CHECK(word_at(search_ram, SS, 0x3EF2u) == 0x3582u);
    CHECK(word_at(search_ram, SS, 0x3EF4u) == 0x9975u);
    CHECK(pr_slot_a_first_exit_run_ram(ram, RAM_SIZE, DS, 16u,
           &state, &env, &report) == PR_SLOT_A_ECL_BIND_OK);
    CHECK(report.run_status == PR_REC_ECL_RUN_OK && report.handler_status == PR_REC_ECL_HANDLER_OK);
    CHECK(report.steps == 10u && report.callback_count == 10u);
    for (i = 0u; i < report.callback_count; ++i) CHECK(report.callback_pc[i] == expected_pc[i]);
    CHECK(report.exit_reached && report.exit_status == PR_U1C_EXIT_OK);
    CHECK(report.exit_report.boxes == 0u && report.exit_report.freed_nodes == 0u);
    CHECK(!report.exit_report.restored_character);
    CHECK(state.pc == 0x9975u && state.latch == 0x00u && state.prev_opcode == 0x2Fu);
    CHECK(state.b442e == 0u && state.b49ff == 0u);
    CHECK(state.b84c8 == 0u && state.b84c9 == 0u && state.b84e0 == 0u && state.b84e2 == 0u);
    CHECK(state.b5e24 == 1u && state.b5e25 == 0x11u);
    CHECK(env.b4430 == 0u && env.b84de == 0u);
    CHECK(env.w5d92 == 8u && env.w5d94 == 0x449Au);
    CHECK(state.pc == word_at(search_ram, DS, 0x49EDu));
    CHECK(state.latch == search_ram[p(DS, 0x6F7Fu)] && state.prev_opcode == search_ram[p(DS, 0x6F7Eu)]);
    CHECK(state.window1[0x382u] == word_at(search_ram, 0x3A16u, 0x704u));
    CHECK(state.b5e24 == search_ram[p(DS, 0x5E24u)] && state.b5e25 == search_ram[p(DS, 0x5E25u)]);
    /* Movement and minute updates between the two native stops are not
       attributed to EXIT by this C composition. */
    CHECK(state.b6aad == 15u && search_ram[p(DS, 0x6AADu)] == 14u);
    CHECK(state.window0[0x18Eu / 2u] == 0u && word_at(search_ram, 0x3996u, 0x18Eu) == 1u);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 1 && argc != 4) return 2;
    if (synthetic()) return 1;
    if (argc == 4 && private_route(argv[1], argv[2], argv[3])) return 1;
    puts("slot-A first ECL EXIT boundary passed");
    return 0;
}
