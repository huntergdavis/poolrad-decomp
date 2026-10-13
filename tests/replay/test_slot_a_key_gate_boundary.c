#include "poolrad/replay/slot_a_key_dispatch_bridge.h"

#include <stdio.h>
#include <string.h>

#define RAM_SIZE 655360u
#define DS_SEG 0x14A0u
#define SS_SEG 0x1D16u

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static uint8_t gate_ram[RAM_SIZE], script_ram[RAM_SIZE];

static size_t physical(unsigned seg, unsigned off)
{
    return ((size_t)seg << 4) + off;
}

static uint16_t word_at(const uint8_t *ram, unsigned seg, unsigned off)
{
    size_t p = physical(seg, off);
    return (uint16_t)((uint16_t)ram[p] | ((uint16_t)ram[p + 1u] << 8));
}

static int load_ram(const char *path, uint8_t *ram)
{
    FILE *file = fopen(path, "rb");
    int complete;
    if (!file) return 0;
    complete = fread(ram, 1u, RAM_SIZE, file) == RAM_SIZE &&
               fgetc(file) == EOF;
    if (fclose(file) != 0) return 0;
    return complete;
}

static int test_synthetic(void)
{
    uint8_t window1[0x800u] = {0};
    uint16_t entry = 0xFFFFu;
    pr_slot_a_key_gate_state state = {window1, sizeof window1,
                                      0x48u, 0u, 0x1234u};
    CHECK(pr_slot_a_key_gate_next(NULL, &entry) == PR_SLOT_A_KEY_GATE_ARGUMENT);
    CHECK(pr_slot_a_key_gate_next(&state, NULL) == PR_SLOT_A_KEY_GATE_ARGUMENT);
    state.window1 = NULL;
    CHECK(pr_slot_a_key_gate_next(&state, &entry) == PR_SLOT_A_KEY_GATE_ARGUMENT);
    state.window1 = window1;
    state.window1_size = 0x595u;
    CHECK(pr_slot_a_key_gate_next(&state, &entry) == PR_SLOT_A_KEY_GATE_ARGUMENT);
    CHECK(entry == 0xFFFFu);

    state.window1_size = 0x596u;
    CHECK(pr_slot_a_key_gate_next(&state, &entry) ==
          PR_SLOT_A_KEY_GATE_BLOCKED_SCRIPT_3171);
    CHECK(entry == 0x1234u);
    window1[0x594u] = 1u;
    CHECK(pr_slot_a_key_gate_next(&state, &entry) ==
          PR_SLOT_A_KEY_GATE_BLOCKED_SCRIPT_3171);
    window1[0x594u] = 2u;
    entry = 0xFFFFu;
    CHECK(pr_slot_a_key_gate_next(&state, &entry) == PR_SLOT_A_KEY_GATE_OUTSIDE_SLICE);
    CHECK(entry == 0xFFFFu);
    window1[0x594u] = 0u;
    window1[0x595u] = 1u;
    CHECK(pr_slot_a_key_gate_next(&state, &entry) == PR_SLOT_A_KEY_GATE_OUTSIDE_SLICE);
    window1[0x595u] = 0u;
    state.local_key_bp_minus_1 = 0x45u;
    CHECK(pr_slot_a_key_gate_next(&state, &entry) == PR_SLOT_A_KEY_GATE_OUTSIDE_SLICE);
    state.local_key_bp_minus_1 = 0x48u;
    state.exit_flag_49ff = 1u;
    CHECK(pr_slot_a_key_gate_next(&state, &entry) == PR_SLOT_A_KEY_GATE_OUTSIDE_SLICE);
    CHECK(entry == 0xFFFFu);
    return 0;
}

static int test_native(const char *gate_path, const char *script_path)
{
    pr_slot_a_key_gate_state state;
    uint16_t entry = 0xFFFFu;
    unsigned w1_seg, w1_off;
    unsigned hits = 0, off;
    CHECK(load_ram(gate_path, gate_ram));
    CHECK(load_ram(script_path, script_ram));
    w1_off = word_at(gate_ram, DS_SEG, 0x49D6u);
    w1_seg = word_at(gate_ram, DS_SEG, 0x49D8u);
    CHECK(w1_seg == 0x3A16u && w1_off == 0u);
    CHECK(physical(w1_seg, w1_off) + 0x596u <= RAM_SIZE);
    state = (pr_slot_a_key_gate_state){gate_ram + physical(w1_seg, w1_off),
            0x800u, gate_ram[physical(SS_SEG, 0x3EF7u)],
            gate_ram[physical(DS_SEG, 0x49FFu)],
            word_at(gate_ram, DS_SEG, 0x49E3u)};
    CHECK(state.local_key_bp_minus_1 == 0x48u);
    CHECK(state.exit_flag_49ff == 0u);
    CHECK(word_at(gate_ram, w1_seg, w1_off + 0x594u) == 0u);
    CHECK(pr_slot_a_key_gate_next(&state, &entry) ==
          PR_SLOT_A_KEY_GATE_BLOCKED_SCRIPT_3171);
    CHECK(entry == 0x9914u);

    CHECK(script_ram[physical(SS_SEG, 0x3EF7u)] == state.local_key_bp_minus_1);
    CHECK(script_ram[physical(DS_SEG, 0x49FFu)] == state.exit_flag_49ff);
    CHECK(word_at(script_ram, DS_SEG, 0x49E3u) == entry);
    CHECK(word_at(script_ram, 0x3A16u, 0x594u) == 0u);
    for (off = 0x3000u; off < 0x4000u; off += 2u) {
        if (word_at(script_ram, SS_SEG, off) == 0x345Bu &&
            word_at(script_ram, SS_SEG, off + 2u) == 0x3582u &&
            word_at(script_ram, SS_SEG, off + 4u) == entry) {
            CHECK(off == 0x3EF0u);
            ++hits;
        }
    }
    CHECK(hits == 1u);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 1 && argc != 3) return 2;
    if (test_synthetic()) return 1;
    if (argc == 3 && test_native(argv[1], argv[2])) return 1;
    puts("slot-A Up key gate to script boundary passed");
    return 0;
}
