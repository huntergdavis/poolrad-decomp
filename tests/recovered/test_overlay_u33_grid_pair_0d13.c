#include "poolrad/recovered/overlay_u33_grid_pair_0d13.h"
#include "poolrad/recovered/overlay_u33_byte_flags_08a4.h"
#include "dice_chain_fixture.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "u33 +0D13 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define GRID_SIZE 0x4E9u
#define COLS 50u

/* The class byte the merged leaf returns for DS:465A = input with a window word of 00FFh. */
/* The merged leaf returns a stack-residue byte for inputs it does not classify,
 * so the probe fills the stack with a chosen residue and callers keep only
 * inputs whose class does not depend on it. */
static uint8_t class_of(uint8_t *ram, uint8_t input, uint8_t residue)
{
    pr_u33_08a4_cpu cpu = {0, 0, 0, 0, 0, 0, 0x7777u, 0x8000u, 0x3000u, 0x08A4u, 0x1000u, 0x1111u, 0x2000u, 0x7202u};
    memset(ram, residue, DICE_RAM);
    ram[dice_physical(0x1000u, 0x465Au)] = input;
    dice_put16(ram, 0x1000u, 0x49D2u, 0u);
    dice_put16(ram, 0x1000u, 0x49D4u, 0x5000u);
    dice_put16(ram, 0x5000u, 0x0366u, 0x00FFu);
    dice_put16(ram, cpu.ss, cpu.sp, 0x7000u);
    dice_put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0x7001u);
    CHECK(pr_u33_byte_flags_08a4(ram, DICE_RAM, &cpu) == PR_U33_08A4_OK);
    return (uint8_t)cpu.ax;
}

static pr_u33_0d13_cpu frame(uint8_t *ram, uint8_t input, uint8_t cell, int eligible, uint32_t seed)
{
    pr_u33_0d13_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
                           0x7777u, 0x8000u, 0x3000u, 0x0D13u, 0x1000u, 0x1111u, 0x2000u, 0x7202u};
    unsigned i;
    memset(ram, 0xA5, DICE_RAM);
    dice_site(ram, 0x3000u, 0x0DD6u, 0x48u, 0x0DE3u);
    dice_site(ram, 0x3000u, 0x0DE9u, 0x48u, 0x0DE3u);
    dice_site(ram, 0x3000u, 0x0DFDu, 0x48u, 0x0DE3u);
    dice_site(ram, 0x3000u, 0x0E27u, 0x48u, 0x0DE3u);
    dice_system(ram, 0x1000u, seed);
    ram[dice_physical(0x1000u, 0x465Au)] = input;
    dice_put16(ram, 0x1000u, 0x49D2u, 0u);
    dice_put16(ram, 0x1000u, 0x49D4u, 0x5000u);
    dice_put16(ram, 0x5000u, 0x0366u, 0x00FFu);
    dice_put16(ram, 0x1000u, 0x6716u, 0u);
    dice_put16(ram, 0x1000u, 0x6718u, 0x8000u);
    for (i = 0u; i < GRID_SIZE; ++i)
        ram[dice_physical(0x8000u, (uint16_t)i)] = cell;
    for (i = 0u; i < 0x400u; ++i)
        ram[dice_physical(0x1000u, (uint16_t)(0x2747u + i))] = 0x17u;
    ram[dice_physical(0x1000u, (uint16_t)(0x2747u + 4u * cell))] = eligible ? 0x16u : 0x17u;
    dice_put16(ram, cpu.ss, cpu.sp, 0x7000u);
    dice_put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0x7001u);
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(DICE_RAM);
    pr_u33_0d13_cpu cpu;
    int chain = -1;
    unsigned input, x, y, writes = 0u;
    uint8_t threshold, class;
    uint32_t seed = 0x0D130D13u;
    CHECK(ram != NULL);

    /* Find an input whose class sets bit 8 (threshold 7) but not bit 80h; derive the threshold. */
    for (input = 0u; input < 256u; ++input) {
        class = class_of(ram, (uint8_t)input, 0xA5u);
        if (class != class_of(ram, (uint8_t)input, 0x00u))
            continue;
        if ((class & 0x08u) != 0u && (class & 0x80u) == 0u)
            break;
    }
    CHECK(input < 256u);
    threshold = 1u;
    if (class & 0x02u) threshold = 1u;
    if (class & 0x04u) threshold = 3u;
    if (class & 0x40u) threshold = 4u;
    if (class & 0x08u) threshold = 7u;

    /* Every cell eligible: for each column and row 1..18h a d100 at or below the threshold
     * rolls the value chain and writes this cell and, below 5, the cell above. */
    cpu = frame(ram, (uint8_t)input, 0x20u, 1, seed);
    CHECK(pr_u33_grid_pair_0d13(ram, DICE_RAM, &cpu, &chain) == PR_U33_0D13_OK && chain == 0);
    {
        uint8_t grid[GRID_SIZE];
        memset(grid, 0x20, sizeof grid);
        for (x = 0u; x <= 0x31u; ++x) {
            for (y = 1u; y <= 0x18u; ++y) {
                uint8_t roll, value;
                /* The original rereads the grid: a cell written below 5 or the cell written
                 * above both leave the eligible byte, so only unwritten pairs roll. */
                if (grid[COLS * y + x + 7u] != 0x20u || grid[COLS * (y - 1u) + x + 7u] != 0x20u)
                    continue;
                roll = dice_roll(&seed, 1u, 100u);
                if (roll > threshold)
                    continue;
                value = dice_roll(&seed, 1u, 10u);
                if (value > 8u)
                    value = (uint8_t)(dice_roll(&seed, 1u, 2u) + 4u);
                else
                    value = (uint8_t)((value + 1u) >> 1);
                if (class & 0x40u)
                    value = (uint8_t)(dice_roll(&seed, 1u, 3u) + 3u);
                grid[COLS * y + x + 7u] = (uint8_t)(value + 0x23u);
                if (value < 5u)
                    grid[COLS * (y - 1u) + x + 7u] = (uint8_t)(value + 0x1Fu);
                ++writes;
            }
        }
        for (x = 0u; x < GRID_SIZE; ++x)
            CHECK(ram[dice_physical(0x8000u, (uint16_t)x)] == grid[x]);
    }
    CHECK(writes > 0u);
    CHECK(dice_seed(ram, 0x1000u) == seed);
    CHECK(cpu.bp == 0x7777u && cpu.sp == 0x8004u && cpu.ip == 0x7000u && cpu.cs == 0x7001u);
    CHECK(cpu.flags == 0x7246u);                        /* CMP column,31h equal */

    /* No cell eligible: the class checks still run (six leaf calls) but no roll happens. */
    cpu = frame(ram, (uint8_t)input, 0x20u, 0, 0x1u);
    CHECK(pr_u33_grid_pair_0d13(ram, DICE_RAM, &cpu, &chain) == PR_U33_0D13_OK && chain == -1);
    CHECK(dice_seed(ram, 0x1000u) == 0x1u);
    for (x = 0u; x < GRID_SIZE; ++x)
        CHECK(ram[dice_physical(0x8000u, (uint16_t)x)] == 0x20u);

    /* A stub that is not a loaded far jump reports CODE at the first roll. */
    cpu = frame(ram, (uint8_t)input, 0x20u, 1, 0x1u);
    ram[dice_physical(DICE_STUB_PARAGRAPH, 0x48u)] = 0xCDu;
    CHECK(pr_u33_grid_pair_0d13(ram, DICE_RAM, &cpu, &chain) == PR_U33_0D13_CODE);

    {
        pr_u33_0d13_cpu before = frame(ram, 0u, 0x20u, 0, 0x1u), after = before;
        CHECK(pr_u33_grid_pair_0d13(NULL, DICE_RAM, &after, NULL) == PR_U33_0D13_ARGUMENT);
        CHECK(pr_u33_grid_pair_0d13(ram, DICE_RAM - 1u, &after, NULL) == PR_U33_0D13_ARGUMENT);
        CHECK(pr_u33_grid_pair_0d13(ram, DICE_RAM, NULL, NULL) == PR_U33_0D13_ARGUMENT);
        CHECK(memcmp(&before, &after, sizeof before) == 0);
    }
    free(ram);
    printf("u33 +0D13: threshold-7 grid, none-eligible, stub and guard cases passed\n");
    return 0;
}
