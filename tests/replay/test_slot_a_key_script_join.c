#include "poolrad/replay/slot_a_key_script_join.h"

#include <stdio.h>
#include <string.h>

#define RAM_SIZE 655360u
#define DS 0x14A0u
#define SS 0x1D16u
#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #x); \
    return 1; \
} } while (0)

static uint8_t before[RAM_SIZE], gate_ram[RAM_SIZE], script_ram[RAM_SIZE];

static size_t physical(unsigned seg, unsigned off)
{
    return ((size_t)seg << 4) + off;
}

static uint16_t word(const uint8_t *ram, unsigned seg, unsigned off)
{
    const uint8_t *at = ram + physical(seg, off);
    return (uint16_t)((uint16_t)at[0] | ((uint16_t)at[1] << 8));
}

static void put_word(uint8_t *ram, unsigned seg, unsigned off, uint16_t value)
{
    uint8_t *at = ram + physical(seg, off);
    at[0] = (uint8_t)value;
    at[1] = (uint8_t)(value >> 8);
}

static int load_ram(const char *path, uint8_t *ram)
{
    FILE *f = fopen(path, "rb");
    int ok;
    if (!f) return 0;
    ok = fread(ram, 1u, RAM_SIZE, f) == RAM_SIZE && fgetc(f) == EOF;
    return fclose(f) == 0 && ok;
}

static int synthetic(void)
{
    uint8_t w0[0x800u] = {0}, w1[0x800u] = {0};
    pr_slot_a_key_return_state returned = {w0, sizeof w0, 0xB7u,
        0x5678u, 0x1234u, 0xBBBBu, 0xCCCCu, 0x45u};
    pr_slot_a_key_join_gate gate = {w1, sizeof w1, 0u, 0x1234u};
    pr_slot_a_key_join_report report = {0};

    CHECK(pr_slot_a_key_script_join(&returned, 0x48u, &gate, &report) ==
          PR_SLOT_A_KEY_JOIN_BLOCKED_SCRIPT_3171);
    CHECK(returned.local_key_bp_minus_1 == 0x48u);
    CHECK(returned.copied_off_4436 == 0x5678u);
    CHECK(returned.copied_seg_4438 == 0x1234u);
    CHECK(w0[0x1E4u] == 0xB7u && w0[0x1E5u] == 0u);
    CHECK(report.return_applied == 1u && report.dependency_valid == 1u);
    CHECK(report.next_pc == 0x3458u && report.dependency.unit == 0x1Cu);
    CHECK(report.dependency.call_pc == 0x3458u);
    CHECK(report.dependency.callee_pc == 0x3171u);
    CHECK(report.dependency.return_pc == 0x345Bu);
    CHECK(report.dependency.entry == 0x1234u);

    /* A new return must replace a prior Up, never reuse a stale gate key. */
    CHECK(pr_slot_a_key_script_join(&returned, 0x45u, &gate, &report) ==
          PR_SLOT_A_KEY_JOIN_OUTSIDE_SLICE);
    CHECK(returned.local_key_bp_minus_1 == 0x45u);
    CHECK(report.return_applied == 1u && report.dependency_valid == 0u);
    CHECK(report.next_pc == 0x33BDu && report.dependency.entry == 0u);
    w1[0x594u] = 2u;
    CHECK(pr_slot_a_key_script_join(&returned, 0x48u, &gate, &report) ==
          PR_SLOT_A_KEY_JOIN_OUTSIDE_SLICE);
    w1[0x594u] = 0u;
    gate.exit_flag_49ff = 1u;
    CHECK(pr_slot_a_key_script_join(&returned, 0x48u, &gate, &report) ==
          PR_SLOT_A_KEY_JOIN_OUTSIDE_SLICE);
    gate.exit_flag_49ff = 0u;

    /* Later failure retains earlier writes and invalidates stale dependency. */
    gate.window1_size = 0x595u;
    returned.local_key_bp_minus_1 = 0xAAu;
    returned.copied_off_4436 = 0xFFFFu;
    CHECK(pr_slot_a_key_script_join(&returned, 0x48u, &gate, &report) ==
          PR_SLOT_A_KEY_JOIN_ARGUMENT);
    CHECK(report.return_applied == 1u && report.dependency_valid == 0u);
    CHECK(returned.local_key_bp_minus_1 == 0x48u);
    CHECK(returned.copied_off_4436 == 0x5678u);

    returned.window0_size = 0x1E5u;
    returned.local_key_bp_minus_1 = 0xAAu;
    CHECK(pr_slot_a_key_script_join(&returned, 0x48u, &gate, &report) ==
          PR_SLOT_A_KEY_JOIN_ARGUMENT);
    CHECK(report.return_applied == 0u && report.next_pc == 0x339Au);
    CHECK(returned.local_key_bp_minus_1 == 0xAAu);
    CHECK(pr_slot_a_key_script_join(NULL, 0x48u, &gate, &report) ==
          PR_SLOT_A_KEY_JOIN_ARGUMENT);
    CHECK(pr_slot_a_key_script_join(&returned, 0x48u, NULL, &report) ==
          PR_SLOT_A_KEY_JOIN_ARGUMENT);
    CHECK(pr_slot_a_key_script_join(&returned, 0x48u, &gate, NULL) ==
          PR_SLOT_A_KEY_JOIN_ARGUMENT);
    return 0;
}

static int native(const char *return_path, const char *gate_path,
                  const char *script_path)
{
    pr_slot_a_key_return_state returned;
    pr_slot_a_key_join_gate gate;
    pr_slot_a_key_join_report report;
    /* Observed at +339A in SLOT_A_NATIVE_KEY_CALL_RETURN.md; not inferred
     * from a save or RAM byte. Only AL is consumed by this state projection. */
    const uint16_t captured_ax = 0xA048u;
    unsigned w0off, w0seg, w1off, w1seg;
    size_t i, changed = 0u;
    CHECK(load_ram(return_path, before));
    CHECK(load_ram(gate_path, gate_ram));
    CHECK(load_ram(script_path, script_ram));
    w0off = word(before, DS, 0x49D2u);
    w0seg = word(before, DS, 0x49D4u);
    w1off = word(before, DS, 0x49D6u);
    w1seg = word(before, DS, 0x49D8u);
    CHECK(w0off == 0u && w0seg == 0x3996u);
    CHECK(w1off == 0u && w1seg == 0x3A16u);
    returned = (pr_slot_a_key_return_state){before + physical(w0seg, w0off),
        0x800u, before[physical(DS, 0x84DCu)],
        word(before, DS, 0x5D92u), word(before, DS, 0x5D94u),
        word(before, DS, 0x4436u), word(before, DS, 0x4438u),
        before[physical(SS, 0x3EF7u)]};
    gate = (pr_slot_a_key_join_gate){before + physical(w1seg, w1off), 0x800u,
        before[physical(DS, 0x49FFu)], word(before, DS, 0x49E3u)};
    CHECK(pr_slot_a_key_script_join(&returned, (uint8_t)captured_ax,
                                   &gate, &report) ==
          PR_SLOT_A_KEY_JOIN_BLOCKED_SCRIPT_3171);
    CHECK(report.return_applied == 1u && report.dependency_valid == 1u);
    CHECK(report.dependency.entry == 0x9914u);

    /* Marshal only the model's outputs. Window 0 already aliases before.
     * All other 640 KiB bytes must equal the same-invocation +33BD dump. */
    before[physical(SS, 0x3EF7u)] = returned.local_key_bp_minus_1;
    put_word(before, DS, 0x4436u, returned.copied_off_4436);
    put_word(before, DS, 0x4438u, returned.copied_seg_4438);
    CHECK(memcmp(before, gate_ram, RAM_SIZE) == 0);
    CHECK(load_ram(return_path, before));
    for (i = 0u; i < RAM_SIZE; ++i)
        if (before[i] != gate_ram[i]) ++changed;
    CHECK(changed == 5u);

    /* Separate same-save invocation: contract check only, no RAM delta claim. */
    CHECK(word(script_ram, SS, 0x3EF0u) == report.dependency.return_pc);
    CHECK(word(script_ram, SS, 0x3EF2u) == 0x3582u);
    CHECK(word(script_ram, SS, 0x3EF4u) == report.dependency.entry);
    CHECK(word(script_ram, DS, 0x49E3u) == report.dependency.entry);
    CHECK(script_ram[physical(SS, 0x3EF7u)] == returned.local_key_bp_minus_1);
    CHECK(script_ram[physical(DS, 0x49FFu)] == gate.exit_flag_49ff);
    CHECK(memcmp(gate.window1, script_ram + physical(w1seg, w1off), 0x800u) == 0);
    puts("native: 655360/655360 return-to-gate RAM bytes match; 5 changed; script contract matches separate run");
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 1 && argc != 4) return 2;
    if (synthetic()) return 1;
    if (argc == 4 && native(argv[1], argv[2], argv[3])) return 1;
    puts("slot-A key/script composition: BLOCKED_SCRIPT_3171 checks passed");
    return 0;
}
