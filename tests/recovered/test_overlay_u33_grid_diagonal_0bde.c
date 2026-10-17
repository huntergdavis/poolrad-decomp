#include "poolrad/recovered/overlay_u33_grid_diagonal_0bde.h"
#include "poolrad/recovered/overlay_u33_byte_flags_08a4.h"
#include "dice_chain_fixture.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "u33 +0BDE line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define GRID_SIZE 0x4E9u
#define COLS 50u

static uint8_t class_of(uint8_t *ram, uint8_t input, uint8_t residue)
{
    pr_u33_08a4_cpu cpu = {0, 0, 0, 0, 0, 0, 0x7777u, 0x8000u, 0x3000u, 0x08A4u, 0x1000u, 0x1111u, 0x2000u, 0x7202u};
    memset(ram, residue, DICE_RAM);
    ram[dice_physical(0x1000u, 0x465Au)] = input;
    dice_put16(ram, 0x1000u, 0x49D2u, 0u);
    dice_put16(ram, 0x1000u, 0x49D4u, 0x5000u);
    dice_put16(ram, 0x5000u, 0x0366u, 0x0000u);
    dice_put16(ram, cpu.ss, cpu.sp, 0x7000u);
    dice_put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0x7001u);
    CHECK(pr_u33_byte_flags_08a4(ram, DICE_RAM, &cpu) == PR_U33_08A4_OK);
    return (uint8_t)cpu.ax;
}

static pr_u33_0bde_cpu frame(uint8_t *ram, uint8_t input, uint32_t seed)
{
    static const uint16_t sites[4] = {0x0C0Eu, 0x0C21u, 0x0C9Cu, 0x0CCEu};
    pr_u33_0bde_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
                           0x7777u, 0x8000u, 0x3000u, 0x0BDEu, 0x1000u, 0x1111u, 0x2000u, 0x7202u};
    unsigned i;
    memset(ram, 0xA5, DICE_RAM);
    for (i = 0u; i < 4u; ++i)
        dice_site(ram, 0x3000u, sites[i], 0x48u, 0x0DE3u);
    dice_system(ram, 0x1000u, seed);
    ram[dice_physical(0x1000u, 0x465Au)] = input;
    dice_put16(ram, 0x1000u, 0x49D2u, 0u);
    dice_put16(ram, 0x1000u, 0x49D4u, 0x5000u);
    dice_put16(ram, 0x5000u, 0x0366u, 0x0000u);
    dice_put16(ram, 0x1000u, 0x6716u, 0u);
    dice_put16(ram, 0x1000u, 0x6718u, 0x8000u);
    for (i = 0u; i < GRID_SIZE; ++i)
        ram[dice_physical(0x8000u, (uint16_t)i)] = 0x20u;
    dice_put16(ram, cpu.ss, cpu.sp, 0x7000u);
    dice_put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0x7001u);
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(DICE_RAM);
    pr_u33_0bde_cpu cpu;
    int chain = -1;
    unsigned input, i;
    uint8_t class, threshold;
    uint32_t seed = 0x0BDE0BDEu;
    CHECK(ram != NULL);

    /* A residue-independent input whose class sets bit 10h gives the threshold 4Bh. */
    for (input = 0u; input < 256u; ++input) {
        class = class_of(ram, (uint8_t)input, 0xA5u);
        if (class != class_of(ram, (uint8_t)input, 0x00u))
            continue;
        if ((class & 0x10u) != 0u)
            break;
    }
    CHECK(input < 256u);
    threshold = 0u;
    if (class & 0x20u) threshold = 0x23u;
    if (class & 0x10u) threshold = 0x4Bu;

    /* Predict the whole routine from the generator: a d100 at or below the threshold draws the
     * diagonal of 32h/33h pairs and places 34h/35h pairs on a d20 of 1 or an odd pair count. */
    {
        uint8_t grid[GRID_SIZE], roll, pairs = 0u, x, saved, row;
        int placed_any = 0;
        memset(grid, 0x20, sizeof grid);
        roll = dice_roll(&seed, 1u, 100u);
        CHECK(roll <= threshold);                        /* this seed passes the gate; keep it meaningful */
        x = (uint8_t)(0x22u - dice_roll(&seed, 5u, 4u));
        while (((x + 2u) % 7u) != 0u)
            --x;
        saved = x;
        for (row = 0u; row <= 0x18u; ++row) {
            if ((int8_t)x <= 0x31) {
                int place;
                grid[COLS * row + x + 7u] = 0x32u;
                grid[COLS * row + x + 1u + 7u] = 0x33u;
                place = dice_roll(&seed, 1u, 0x14u) == 1u || (pairs & 1u) != 0u;
                if (place) {
                    grid[COLS * row + x + 7u] = 0x34u;
                    grid[COLS * row + x + 1u + 7u] = 0x35u;
                    ++pairs;
                    placed_any = 1;
                }
                ++x;
            }
        }
        if (!placed_any) {
            uint8_t r = (uint8_t)(0x0Cu - dice_roll(&seed, 1u, 9u) + 4u), cx = (uint8_t)(saved + r);
            grid[COLS * r + cx + 7u] = 0x34u; grid[COLS * r + cx + 1u + 7u] = 0x35u;
            grid[COLS * (r + 1u) + cx + 1u + 7u] = 0x34u; grid[COLS * (r + 1u) + cx + 2u + 7u] = 0x35u;
        }
        cpu = frame(ram, (uint8_t)input, 0x0BDE0BDEu);
        CHECK(pr_u33_grid_diagonal_0bde(ram, DICE_RAM, &cpu, &chain) == PR_U33_0BDE_OK && chain == 0);
        for (i = 0u; i < GRID_SIZE; ++i)
            CHECK(ram[dice_physical(0x8000u, (uint16_t)i)] == grid[i]);
        CHECK(dice_seed(ram, 0x1000u) == seed);
        CHECK(ram[dice_physical(0x2000u, 0x7FFCu)] == (placed_any ? pairs : 2u));   /* [BP-2] pair count */
    }
    CHECK(cpu.bp == 0x7777u && cpu.sp == 0x8004u && cpu.ip == 0x7000u && cpu.cs == 0x7001u);

    /* A seed whose first d100 exceeds the threshold returns after one roll with nothing drawn. */
    {
        uint32_t probe = 0x1u;
        unsigned tries = 0u;
        while (dice_roll(&probe, 1u, 100u) <= threshold && tries++ < 64u)
            ;
        CHECK(tries < 64u);
        cpu = frame(ram, (uint8_t)input, 0x1u);
        /* advance the fixture seed to the state before the exceeding roll */
        {
            uint32_t s = 0x1u;
            unsigned k;
            for (k = 0u; k < tries; ++k)
                dice_roll(&s, 1u, 100u);
            dice_system(ram, 0x1000u, s);
        }
        CHECK(pr_u33_grid_diagonal_0bde(ram, DICE_RAM, &cpu, &chain) == PR_U33_0BDE_OK);
        for (i = 0u; i < GRID_SIZE; ++i)
            CHECK(ram[dice_physical(0x8000u, (uint16_t)i)] == 0x20u);
    }

    cpu = frame(ram, (uint8_t)input, 0x1u);
    ram[dice_physical(DICE_STUB_PARAGRAPH, 0x48u)] = 0xCDu;
    CHECK(pr_u33_grid_diagonal_0bde(ram, DICE_RAM, &cpu, &chain) == PR_U33_0BDE_CODE);
    {
        pr_u33_0bde_cpu before = frame(ram, 0u, 0x1u), after = before;
        CHECK(pr_u33_grid_diagonal_0bde(NULL, DICE_RAM, &after, NULL) == PR_U33_0BDE_ARGUMENT);
        CHECK(pr_u33_grid_diagonal_0bde(ram, DICE_RAM - 1u, &after, NULL) == PR_U33_0BDE_ARGUMENT);
        CHECK(pr_u33_grid_diagonal_0bde(ram, DICE_RAM, NULL, NULL) == PR_U33_0BDE_ARGUMENT);
        CHECK(memcmp(&before, &after, sizeof before) == 0);
    }
    free(ram);
    printf("u33 +0BDE: diagonal with pairs, gated-out seed, stub and guard cases passed\n");
    return 0;
}
