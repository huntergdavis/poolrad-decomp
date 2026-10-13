#include "poolrad/replay/slot_a_key_dispatch_bridge.h"

#include <stdio.h>
#include <string.h>

#define RAM_SIZE 655360u
#define WINDOW0_SIZE 0x800u
#define DS_SEG 0x14A0u
#define SS_SEG 0x1D16u

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static uint8_t before_ram[RAM_SIZE], gate_ram[RAM_SIZE];

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
    int extra;
    if (!file) return 0;
    extra = fread(ram, 1u, RAM_SIZE, file) == RAM_SIZE &&
            fgetc(file) == EOF;
    if (fclose(file) != 0) return 0;
    return extra;
}

static int test_synthetic(void)
{
    uint8_t window0[WINDOW0_SIZE], saved_window0[WINDOW0_SIZE];
    pr_slot_a_key_return_state state, saved;
    memset(window0, 0xA5, sizeof window0);
    state = (pr_slot_a_key_return_state){window0, sizeof window0, 0xB7u,
            0x5678u, 0x1234u, 0xBBBBu, 0xCCCCu, 0xAAu};
    saved = state;
    memcpy(saved_window0, window0, sizeof window0);
    CHECK(pr_slot_a_key_return_step(NULL, 0x48u) == PR_SLOT_A_KEY_RETURN_ARGUMENT);
    state.window0 = NULL;
    CHECK(pr_slot_a_key_return_step(&state, 0x48u) == PR_SLOT_A_KEY_RETURN_ARGUMENT);
    state.window0 = window0;
    state.window0_size = 0x1E5u;
    CHECK(pr_slot_a_key_return_step(&state, 0x48u) == PR_SLOT_A_KEY_RETURN_ARGUMENT);
    CHECK(state.local_key_bp_minus_1 == saved.local_key_bp_minus_1);
    CHECK(state.copied_off_4436 == saved.copied_off_4436);
    CHECK(state.copied_seg_4438 == saved.copied_seg_4438);
    CHECK(memcmp(window0, saved_window0, sizeof window0) == 0);

    state.window0_size = 0x1E6u;
    CHECK(pr_slot_a_key_return_step(&state, 0x48u) == PR_SLOT_A_KEY_RETURN_OK);
    CHECK(state.local_key_bp_minus_1 == 0x48u);
    CHECK(state.copied_off_4436 == 0x5678u && state.copied_seg_4438 == 0x1234u);
    CHECK(window0[0x1E4u] == 0xB7u && window0[0x1E5u] == 0u);
    window0[0x1E4u] = saved_window0[0x1E4u];
    window0[0x1E5u] = saved_window0[0x1E5u];
    CHECK(memcmp(window0, saved_window0, sizeof window0) == 0);
    CHECK(state.map_id_84dc == saved.map_id_84dc);
    CHECK(state.active_off_5d92 == saved.active_off_5d92);
    CHECK(state.active_seg_5d94 == saved.active_seg_5d94);
    CHECK(pr_slot_a_key_return_step(&state, 0x00u) == PR_SLOT_A_KEY_RETURN_OK);
    CHECK(state.local_key_bp_minus_1 == 0x00u);
    return 0;
}

static int test_native(const char *before_path, const char *gate_path)
{
    uint8_t live_window0[WINDOW0_SIZE];
    pr_slot_a_key_return_state state;
    unsigned w0_seg, w0_off;
    size_t key_p = physical(SS_SEG, 0x3EF7u);
    size_t i, changed = 0;
    CHECK(load_ram(before_path, before_ram));
    CHECK(load_ram(gate_path, gate_ram));
    w0_off = word_at(before_ram, DS_SEG, 0x49D2u);
    w0_seg = word_at(before_ram, DS_SEG, 0x49D4u);
    CHECK(w0_seg == 0x3996u && w0_off == 0u);
    CHECK(word_at(gate_ram, DS_SEG, 0x49D2u) == w0_off);
    CHECK(word_at(gate_ram, DS_SEG, 0x49D4u) == w0_seg);
    CHECK(physical(w0_seg, w0_off) + WINDOW0_SIZE <= RAM_SIZE);
    memcpy(live_window0, before_ram + physical(w0_seg, w0_off), WINDOW0_SIZE);
    state = (pr_slot_a_key_return_state){live_window0, WINDOW0_SIZE,
            before_ram[physical(DS_SEG, 0x84DCu)],
            word_at(before_ram, DS_SEG, 0x5D92u),
            word_at(before_ram, DS_SEG, 0x5D94u),
            word_at(before_ram, DS_SEG, 0x4436u),
            word_at(before_ram, DS_SEG, 0x4438u),
            before_ram[key_p]};
    CHECK(state.map_id_84dc == gate_ram[physical(DS_SEG, 0x84DCu)]);
    CHECK(state.active_off_5d92 == word_at(gate_ram, DS_SEG, 0x5D92u));
    CHECK(state.active_seg_5d94 == word_at(gate_ram, DS_SEG, 0x5D94u));
    CHECK(state.local_key_bp_minus_1 == 0xA0u);
    CHECK(pr_slot_a_key_return_step(&state, 0x48u) == PR_SLOT_A_KEY_RETURN_OK);
    CHECK(state.local_key_bp_minus_1 == gate_ram[key_p]);
    CHECK(state.copied_off_4436 == word_at(gate_ram, DS_SEG, 0x4436u));
    CHECK(state.copied_seg_4438 == word_at(gate_ram, DS_SEG, 0x4438u));
    CHECK(memcmp(live_window0, gate_ram + physical(w0_seg, w0_off), WINDOW0_SIZE) == 0);
    for (i = 0; i < RAM_SIZE; ++i) {
        if (before_ram[i] != gate_ram[i]) {
            CHECK(i == key_p ||
                  (i >= physical(DS_SEG, 0x4436u) &&
                   i <= physical(DS_SEG, 0x4439u)));
            ++changed;
        }
    }
    CHECK(changed == 5u);
    CHECK(key_p == physical(DS_SEG, 0xC657u));
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 1 && argc != 3) return 2;
    if (test_synthetic()) return 1;
    if (argc == 3 && test_native(argv[1], argv[2])) return 1;
    puts("slot-A key-return bridge checks passed");
    return 0;
}
