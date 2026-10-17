#include "poolrad/recovered/overlay_u33_grid_roll_0000.h"
#include "dice_chain_fixture.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "u33 +0000 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define GRID_SIZE 0x4E9u

/* The grid at 8000:0000 holds cell byte `cell` everywhere; the table marks it eligible or not. */
static pr_u33_0000_cpu frame(uint8_t *ram, uint8_t cell, int eligible, uint8_t map_id, uint32_t seed)
{
    pr_u33_0000_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
                           0x7777u, 0x8000u, 0x3000u, 0x0000u, 0x1000u, 0x1111u, 0x2000u, 0x7202u};
    unsigned i;
    memset(ram, 0xA5, DICE_RAM);
    dice_site(ram, 0x3000u, 0x004Bu, 0x48u, 0x0DE3u);
    dice_site(ram, 0x3000u, 0x00B7u, 0x48u, 0x0DE3u);
    dice_site(ram, 0x3000u, 0x00C6u, 0x48u, 0x0DE3u);
    dice_system(ram, 0x1000u, seed);
    dice_put16(ram, 0x1000u, 0x6716u, 0u);
    dice_put16(ram, 0x1000u, 0x6718u, 0x8000u);
    for (i = 0u; i < GRID_SIZE; ++i)
        ram[dice_physical(0x8000u, (uint16_t)i)] = cell;
    for (i = 0u; i < 0x400u; ++i)
        ram[dice_physical(0x1000u, (uint16_t)(0x2747u + i))] = 0x17u;
    ram[dice_physical(0x1000u, (uint16_t)(0x2747u + 4u * cell))] = eligible ? 0x16u : 0x17u;
    ram[dice_physical(0x1000u, 0x84DCu)] = map_id;
    dice_put16(ram, cpu.ss, cpu.sp, 0x7000u);
    dice_put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0x7001u);
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(DICE_RAM);
    pr_u33_0000_cpu cpu;
    int chain = -1;
    uint32_t seed = 0x600DF00Du;
    unsigned x, y, writes = 0u;
    CHECK(ram != NULL);

    /* Every cell eligible: 1,250 d100 rolls in column-major order; 98 -> 1Ah, 99 -> 1Bh, 100 -> a
     * second d100 that must be 1 and then a d10: 1..6 -> 1Ch, 7..10 -> 1Dh. The same generator
     * predicts each cell, and every other cell keeps its byte. */
    cpu = frame(ram, 0x20u, 1, 5u, 0x600DF00Du);
    CHECK(pr_u33_grid_roll_0000(ram, DICE_RAM, &cpu, &chain) == PR_U33_0000_OK && chain == 0);
    for (x = 0u; x <= 0x31u; ++x) {
        for (y = 0u; y <= 0x18u; ++y) {
            uint8_t roll = dice_roll(&seed, 1u, 100u), expect = 0x20u;
            if (roll == 0x62u) expect = 0x1Au;
            else if (roll == 0x63u) expect = 0x1Bu;
            else if (roll == 0x64u && dice_roll(&seed, 1u, 100u) == 1u) {
                uint8_t ten = dice_roll(&seed, 1u, 10u);
                expect = ten <= 6u ? 0x1Cu : 0x1Du;
            }
            if (expect != 0x20u) ++writes;
            CHECK(ram[dice_physical(0x8000u, (uint16_t)(50u * y + x + 7u))] == expect);
        }
    }
    CHECK(writes > 0u);
    CHECK(dice_seed(ram, 0x1000u) == seed);
    CHECK(cpu.bp == 0x7777u);                                           /* BX and SI follow the generator */
    CHECK(cpu.sp == 0x8004u && cpu.ip == 0x7000u && cpu.cs == 0x7001u);
    CHECK(cpu.flags == 0x7246u);                        /* CMP column,31h equal */
    CHECK(ram[dice_physical(0x2000u, 0x7FFDu)] == 0x31u && ram[dice_physical(0x2000u, 0x7FFCu)] == 0x18u);

    /* No cell eligible: no roll, the seed and grid are untouched. */
    cpu = frame(ram, 0x20u, 0, 5u, 0x1u);
    CHECK(pr_u33_grid_roll_0000(ram, DICE_RAM, &cpu, &chain) == PR_U33_0000_OK && chain == -1);
    CHECK(dice_seed(ram, 0x1000u) == 0x1u);
    CHECK(cpu.bx == 0x3456u && cpu.si == 0x9ABCu);                         /* no generator ran */
    for (x = 0u; x < GRID_SIZE; ++x)
        CHECK(ram[dice_physical(0x8000u, (uint16_t)x)] == 0x20u);

    /* A stub that is not a loaded far jump reports CODE at the first eligible cell. */
    cpu = frame(ram, 0x20u, 1, 5u, 0x1u);
    ram[dice_physical(DICE_STUB_PARAGRAPH, 0x48u)] = 0xCDu;
    CHECK(pr_u33_grid_roll_0000(ram, DICE_RAM, &cpu, &chain) == PR_U33_0000_CODE);

    {
        pr_u33_0000_cpu before = frame(ram, 0x20u, 0, 5u, 0x1u), after = before;
        CHECK(pr_u33_grid_roll_0000(NULL, DICE_RAM, &after, NULL) == PR_U33_0000_ARGUMENT);
        CHECK(pr_u33_grid_roll_0000(ram, DICE_RAM - 1u, &after, NULL) == PR_U33_0000_ARGUMENT);
        CHECK(pr_u33_grid_roll_0000(ram, DICE_RAM, NULL, NULL) == PR_U33_0000_ARGUMENT);
        CHECK(memcmp(&before, &after, sizeof before) == 0);
    }
    free(ram);
    printf("u33 +0000: all-eligible grid, none-eligible, stub and guard cases passed\n");
    return 0;
}
