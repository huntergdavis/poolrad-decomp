#include "poolrad/recovered/overlay_u33_grid_bands_0e9e.h"
#include "poolrad/recovered/overlay_u33_byte_flags_08a4.h"
#include "dice_chain_fixture.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "u33 +0E9E line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define GRID_SIZE 0x4E9u
#define COLS 50u

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
    dice_put16(ram, 0x5000u, 0x0366u, 0x0000u);   /* a window word of 00FFh would clear the high class bit */
    dice_put16(ram, cpu.ss, cpu.sp, 0x7000u);
    dice_put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0x7001u);
    CHECK(pr_u33_byte_flags_08a4(ram, DICE_RAM, &cpu) == PR_U33_08A4_OK);
    return (uint8_t)cpu.ax;
}

static pr_u33_0e9e_cpu frame(uint8_t *ram, uint8_t input, uint8_t cell, int eligible, uint32_t seed)
{
    static const uint16_t sites[7] = {0x0F45u, 0x0F5Eu, 0x1026u, 0x107Bu, 0x10D9u, 0x1150u, 0x116Eu};
    pr_u33_0e9e_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
                           0x7777u, 0x8000u, 0x3000u, 0x0E9Eu, 0x1000u, 0x1111u, 0x2000u, 0x7202u};
    unsigned i;
    memset(ram, 0xA5, DICE_RAM);
    for (i = 0u; i < 7u; ++i)
        dice_site(ram, 0x3000u, sites[i], 0x48u, 0x0DE3u);
    dice_system(ram, 0x1000u, seed);
    ram[dice_physical(0x1000u, 0x465Au)] = input;
    dice_put16(ram, 0x1000u, 0x49D2u, 0u);
    dice_put16(ram, 0x1000u, 0x49D4u, 0x5000u);
    dice_put16(ram, 0x5000u, 0x0366u, 0x0000u);
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
    pr_u33_0e9e_cpu cpu;
    int chain = -1;
    unsigned input, x, y, writes = 0u;
    uint8_t class, b1, b2, b3, b4, b5;
    uint32_t seed = 0x0E9E0E9Eu;
    CHECK(ram != NULL);

    /* An input whose class sets bit 80h gives the first band a width of 0Fh. */
    for (input = 0u; input < 256u; ++input) {
        class = class_of(ram, (uint8_t)input, 0xA5u);
        if (class != class_of(ram, (uint8_t)input, 0x00u))
            continue;
        if ((class & 0x80u) != 0u)
            break;
    }
    CHECK(input < 256u);
    b1 = 0u; b2 = 6u; b3 = 0x0Fu; b4 = 0x28u; b5 = 0u;
    if (class & 0x02u) { b3 = 0x2Du; b4 = 0x0Au; }
    if (class & 0x04u) b3 = 0x19u;
    if (class & 0x40u) { b4 = 5u; b5 = 0x1Eu; }
    if (class & 0x80u) { b1 = 0x0Fu; b4 = 0x19u; b3 = 0x0Au; }

    /* Every cell eligible: a (1, 0FFh) roll per cell in column-major order picks the band. Since all
     * cells share one byte, the cell above is eligible wherever row > 0 at the moment of the check. */
    cpu = frame(ram, (uint8_t)input, 0x20u, 1, seed);
    CHECK(pr_u33_grid_bands_0e9e(ram, DICE_RAM, &cpu, &chain) == PR_U33_0E9E_OK && chain == 0);
    {
        uint8_t grid[GRID_SIZE];
        memset(grid, 0x20, sizeof grid);
        for (x = 0u; x <= 0x31u; ++x) {
            for (y = 0u; y <= 0x18u; ++y) {
                uint8_t roll = dice_roll(&seed, 1u, 255u), value;
                unsigned here = COLS * y + x + 7u, above = here - COLS;
                int above_eligible;
                if (roll <= b1) {
                    value = dice_roll(&seed, 1u, 4u);
                    above_eligible = y > 0u && ram[dice_physical(0x1000u, (uint16_t)(0x2747u + 4u * grid[above]))] == 0x16u;
                    if (value == 4u && above_eligible) { grid[above] = 0x41u; grid[here] = 0x40u; }
                    if (value < 4u) grid[here] = (uint8_t)(value + 0x3Cu);
                } else if (roll <= b1 + b2) {
                    grid[here] = (uint8_t)(dice_roll(&seed, 1u, 3u) + 0x36u);
                } else if (roll <= b1 + b2 + b3) {
                    grid[here] = (uint8_t)(dice_roll(&seed, 1u, 4u) + 0x29u);
                } else if (roll <= b1 + b2 + b3 + b5) {
                    value = (uint8_t)((dice_roll(&seed, 1u, 10u) - 1u) / 3u + 1u);
                    grid[here] = (uint8_t)(value + 0x38u);
                } else if (roll <= b1 + b2 + b3 + b5 + b4) {
                    value = dice_roll(&seed, 1u, 4u);
                    if (value == 4u && (class & 0x40u) == 0u)
                        value = dice_roll(&seed, 1u, 3u);
                    grid[here] = (uint8_t)(value + 0x2Du);
                } else {
                    continue;
                }
                ++writes;
            }
        }
        for (x = 0u; x < GRID_SIZE; ++x)
            CHECK(ram[dice_physical(0x8000u, (uint16_t)x)] == grid[x]);
    }
    CHECK(writes > 0u);
    CHECK(dice_seed(ram, 0x1000u) == seed);
    CHECK(cpu.bp == 0x7777u && cpu.sp == 0x8004u && cpu.ip == 0x7000u && cpu.cs == 0x7001u);
    CHECK(cpu.flags == 0x7246u);

    /* No cell eligible: four leaf calls, no roll, the grid untouched. */
    cpu = frame(ram, (uint8_t)input, 0x20u, 0, 0x1u);
    CHECK(pr_u33_grid_bands_0e9e(ram, DICE_RAM, &cpu, &chain) == PR_U33_0E9E_OK && chain == -1);
    CHECK(dice_seed(ram, 0x1000u) == 0x1u);
    for (x = 0u; x < GRID_SIZE; ++x)
        CHECK(ram[dice_physical(0x8000u, (uint16_t)x)] == 0x20u);

    cpu = frame(ram, (uint8_t)input, 0x20u, 1, 0x1u);
    ram[dice_physical(DICE_STUB_PARAGRAPH, 0x48u)] = 0xCDu;
    CHECK(pr_u33_grid_bands_0e9e(ram, DICE_RAM, &cpu, &chain) == PR_U33_0E9E_CODE);

    {
        pr_u33_0e9e_cpu before = frame(ram, 0u, 0x20u, 0, 0x1u), after = before;
        CHECK(pr_u33_grid_bands_0e9e(NULL, DICE_RAM, &after, NULL) == PR_U33_0E9E_ARGUMENT);
        CHECK(pr_u33_grid_bands_0e9e(ram, DICE_RAM - 1u, &after, NULL) == PR_U33_0E9E_ARGUMENT);
        CHECK(pr_u33_grid_bands_0e9e(ram, DICE_RAM, NULL, NULL) == PR_U33_0E9E_ARGUMENT);
        CHECK(memcmp(&before, &after, sizeof before) == 0);
    }
    free(ram);
    printf("u33 +0E9E: banded grid, none-eligible, stub and guard cases passed\n");
    return 0;
}
