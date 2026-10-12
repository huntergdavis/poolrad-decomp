#include "poolrad/formats/dos_savgam.h"
#include "poolrad/replay/slot_a_post_transfer.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #x); \
    return 1; \
} } while (0)

static int synthetic(void)
{
    static pr_slot_a_native_state world;
    pr_slot_a_post_transfer_ds ds;
    unsigned word;
    memset(&world, 0xA5, sizeof world);
    memset(&ds, 0xA5, sizeof ds);
    CHECK(pr_slot_a_post_transfer_apply(NULL, &ds) == PR_SLOT_A_POST_TRANSFER_ARGUMENT);
    CHECK(ds.b49f6 == 0xA5u && ds.b49f5 == 0xA5u && ds.b49e2 == 0xA5u);
    CHECK(pr_slot_a_post_transfer_apply(&world, NULL) == PR_SLOT_A_POST_TRANSFER_ARGUMENT);
    CHECK(world.window1[PR_SLOT_A_POST_W1_WORD_67C] == 0xA5u);

    /* All 16-bit source words: shift before narrowing, split parity, and
       clear exactly two bytes while preserving the adjacent window bytes. */
    for (word = 0u; word <= 0xFFFFu; ++word) {
        world.window0[PR_SLOT_A_POST_W0_WORD_1FE] = (uint8_t)word;
        world.window0[PR_SLOT_A_POST_W0_WORD_1FE + 1u] = (uint8_t)(word >> 8);
        world.window0[PR_SLOT_A_POST_W0_BYTE_1F8] = (uint8_t)(word ^ 0x5Au);
        world.window1[PR_SLOT_A_POST_W1_WORD_67C] = 0xE1u;
        world.window1[PR_SLOT_A_POST_W1_WORD_67C + 1u] = 0xE2u;
        CHECK(pr_slot_a_post_transfer_apply(&world, &ds) == PR_SLOT_A_POST_TRANSFER_OK);
        CHECK(ds.b49f6 == (uint8_t)(word >> 1));
        CHECK(ds.b49f5 == (uint8_t)(word & 1u));
        CHECK(ds.b49e2 == (uint8_t)(word ^ 0x5Au));
        CHECK(world.window1[PR_SLOT_A_POST_W1_WORD_67C] == 0u &&
              world.window1[PR_SLOT_A_POST_W1_WORD_67C + 1u] == 0u);
        CHECK(world.window1[PR_SLOT_A_POST_W1_WORD_67C - 1u] == 0xA5u &&
              world.window1[PR_SLOT_A_POST_W1_WORD_67C + 2u] == 0xA5u);
    }
    return 0;
}

static int private_save(const char *path)
{
    static pr_slot_a_native_state world;
    uint8_t data[PR_DOS_SAVGAM_SIZE];
    uint8_t w0_before[PR_SLOT_A_WINDOW0_SIZE];
    uint8_t w1_before[PR_SLOT_A_WINDOW1_SIZE];
    pr_slot_a_post_transfer_ds ds = {0xFFu, 0xFFu, 0xFFu};
    FILE *f = fopen(path, "rb");
    CHECK(f != NULL);
    CHECK(fread(data, 1u, sizeof data, f) == sizeof data);
    CHECK(fgetc(f) == EOF);
    CHECK(fclose(f) == 0);
    CHECK(pr_slot_a_native_restore(data, sizeof data, &world) == PR_SLOT_A_NATIVE_OK);
    memcpy(w0_before, world.window0, sizeof w0_before);
    memcpy(w1_before, world.window1, sizeof w1_before);
    CHECK(world.window0[0x1FEu] == 3u && world.window0[0x1FFu] == 0u);
    CHECK(world.window0[0x1F8u] == 4u);
    CHECK(world.window1[0x67Cu] == 6u && world.window1[0x67Du] == 0u);
    CHECK(pr_slot_a_post_transfer_apply(&world, &ds) == PR_SLOT_A_POST_TRANSFER_OK);
    CHECK(ds.b49f6 == 1u && ds.b49f5 == 1u && ds.b49e2 == 4u);
    CHECK(memcmp(world.window0, w0_before, sizeof w0_before) == 0);
    w1_before[0x67Cu] = 0u;
    w1_before[0x67Du] = 0u;
    CHECK(memcmp(world.window1, w1_before, sizeof w1_before) == 0);
    CHECK(world.party[0] == 15u && world.party[1] == 4u && world.party[2] == 6u);
    CHECK(world.post_load_selector == 1u && world.map_id == 20u);
    puts("saved post-transfer: DS 49F6/49F5/49E2 = 1/1/4, window1 +67C cleared");
    return 0;
}

int main(int argc, char **argv)
{
    if (argc > 2 || synthetic() != 0) return 1;
    if (argc == 2 && private_save(argv[1]) != 0) return 1;
    puts("synthetic post-transfer word sweep passed");
    return 0;
}
