#include "poolrad/recovered/overlay_u33_grid_driver_11b4.h"
#include "dice_chain_fixture.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "u33 +11B4 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define GRID_SIZE 0x4E9u

/* All eighteen dice call sites of the composed bodies, at their offsets below CS:11B4. */
static const uint16_t sites[18] = {0x004Bu, 0x00B7u, 0x00C6u, 0x0C0Eu, 0x0C21u, 0x0C9Cu, 0x0CCEu, 0x0DD6u, 0x0DE9u,
                                   0x0DFDu, 0x0E27u, 0x0F45u, 0x0F5Eu, 0x1026u, 0x107Bu, 0x10D9u, 0x1150u, 0x116Eu};

static pr_u33_11b4_cpu frame(uint8_t *ram, uint8_t first, uint8_t second, uint8_t mode, uint32_t seed)
{
    pr_u33_11b4_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
                           0x7777u, 0x8000u, 0x3000u, 0x11B4u, 0x1000u, 0x1111u, 0x2000u, 0x7202u};
    unsigned i;
    memset(ram, 0xA5, DICE_RAM);
    for (i = 0u; i < 18u; ++i)
        dice_site(ram, 0x3000u, sites[i], 0x48u, 0x0DE3u);
    /* the FillChar site: 9A A8 16 F8 0A (absolute SYSTEM paragraph, unrelocated here) */
    ram[dice_physical(0x3000u, 0x11CBu)] = 0x9Au;
    dice_put16(ram, 0x3000u, 0x11CCu, 0x16A8u);
    dice_put16(ram, 0x3000u, 0x11CEu, 0x0AF8u);
    dice_system(ram, 0x1000u, seed);
    dice_put16(ram, 0x1000u, 0x49D2u, 0u);
    dice_put16(ram, 0x1000u, 0x49D4u, 0x5000u);
    dice_put16(ram, 0x5000u, 0x0366u, 0x0000u);
    ram[dice_physical(0x5000u, 0x0186u)] = first;
    ram[dice_physical(0x5000u, 0x0188u)] = second;
    ram[dice_physical(0x1000u, 0x49FAu)] = mode;
    for (i = 0u; i < 0x100u; ++i)
        ram[dice_physical(0x1000u, (uint16_t)(0x3648u + i))] = (uint8_t)(0x30u + i);
    dice_put16(ram, 0x1000u, 0x6716u, 0u);
    dice_put16(ram, 0x1000u, 0x6718u, 0x8000u);
    for (i = 0u; i < 0x400u; ++i)
        ram[dice_physical(0x1000u, (uint16_t)(0x2747u + i))] = 0x17u;   /* no cell eligible after the fill */
    dice_put16(ram, cpu.ss, cpu.sp, 0x7000u);
    dice_put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0x7001u);
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(DICE_RAM);
    pr_u33_11b4_cpu cpu;
    int chain = -1;
    unsigned i;
    CHECK(ram != NULL);

    /* With no eligible cell the four generators still run their class checks and +0BDE's
     * gate roll; the grid is the FillChar result and the class byte the table lookup
     * 3648 + 2Ch * second + first (+ 0Dh for mode 3, + 1Ah for mode 4). */
    cpu = frame(ram, 2u, 1u, 3u, 0x11B411B4u);
    CHECK(pr_u33_grid_driver_11b4(ram, DICE_RAM, &cpu, &chain) == PR_U33_11B4_OK);
    for (i = 0u; i < 7u; ++i)
        CHECK(ram[dice_physical(0x8000u, (uint16_t)i)] == 0xA5u);
    for (i = 7u; i < GRID_SIZE; ++i)
        CHECK(ram[dice_physical(0x8000u, (uint16_t)i)] == 0x17u);
    CHECK(ram[dice_physical(0x8000u, GRID_SIZE)] == 0xA5u);
    CHECK(ram[dice_physical(0x1000u, 0x465Au)] == (uint8_t)(0x30u + 0x2Cu * 1u + 2u + 0x0Du));
    CHECK(cpu.bp == 0x7777u && cpu.sp == 0x8004u && cpu.ip == 0x7000u && cpu.cs == 0x7001u);

    cpu = frame(ram, 5u, 0u, 4u, 0x11B411B4u);
    CHECK(pr_u33_grid_driver_11b4(ram, DICE_RAM, &cpu, &chain) == PR_U33_11B4_OK);
    CHECK(ram[dice_physical(0x1000u, 0x465Au)] == (uint8_t)(0x30u + 5u + 0x1Au));

    /* A corrupted FillChar site reports CODE before any fill. */
    cpu = frame(ram, 2u, 1u, 0u, 0x1u);
    ram[dice_physical(0x3000u, 0x11CBu)] = 0u;
    CHECK(pr_u33_grid_driver_11b4(ram, DICE_RAM, &cpu, &chain) == PR_U33_11B4_CODE);
    CHECK(ram[dice_physical(0x8000u, 7u)] == 0xA5u);

    {
        pr_u33_11b4_cpu before = frame(ram, 0u, 0u, 0u, 0x1u), after = before;
        CHECK(pr_u33_grid_driver_11b4(NULL, DICE_RAM, &after, NULL) == PR_U33_11B4_ARGUMENT);
        CHECK(pr_u33_grid_driver_11b4(ram, DICE_RAM - 1u, &after, NULL) == PR_U33_11B4_ARGUMENT);
        CHECK(pr_u33_grid_driver_11b4(ram, DICE_RAM, NULL, NULL) == PR_U33_11B4_ARGUMENT);
        CHECK(memcmp(&before, &after, sizeof before) == 0);
    }
    free(ram);
    printf("u33 +11B4: fill and class lookup, mode 4, site and guard cases passed\n");
    return 0;
}
