#include "poolrad/recovered/overlay_u33_neighbourhood_0810.h"
#include "dice_chain_fixture.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "u33 +0810 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

/* Synthetic loaded-state metadata only: the +012B far-call sites and u3BD stubs, the three +0000
 * dice sites and stub, the SYSTEM generator's seed and multiplier, and zeroed lookup tables. */
static pr_u33_0810_cpu enter(uint8_t *ram, uint8_t eligible, uint32_t seed)
{
    pr_u33_0810_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
                           0x7777u, 0x8000u, 0x3000u, 0x0810u, 0x1000u, 0x1111u, 0x2000u, 0x7202u};
    unsigned i;
    memset(ram, 0xA5, DICE_RAM);
    memset(ram + dice_physical(0x5000u, 0x0100u), 0, 0x400u);           /* +012B buffer */
    memset(ram + dice_physical(0x6000u, 0x0100u), 0, 0x4F0u);           /* grid */
    memset(ram + dice_physical(0x1000u, 0x2736u), 0, 0x10Au);           /* lookup and step tables */
    for (i = 0; i < 0x100u; ++i)                                        /* +0000 eligibility table */
        ram[dice_physical(0x1000u, (uint16_t)(0x2747u + 4u * i))] = eligible ? 0x16u : 0x17u;
    ram[dice_physical(0x1000u, 0x84DCu)] = 5u;
    ram[dice_physical(0x1000u, 0x6AADu)] = 5u;
    ram[dice_physical(0x1000u, 0x6AAEu)] = 3u;
    dice_put16(ram, 0x1000u, 0x6A5Cu, 0x0100u); dice_put16(ram, 0x1000u, 0x6A5Eu, 0x5000u);
    dice_put16(ram, 0x1000u, 0x6716u, 0x0100u); dice_put16(ram, 0x1000u, 0x6718u, 0x6000u);
    ram[dice_physical(0x3000u, 0x0155u)] = 0x9Au; dice_put16(ram, 0x3000u, 0x0156u, 0x2Fu); dice_put16(ram, 0x3000u, 0x0158u, 0x3D5u);
    ram[dice_physical(0x3000u, 0x0170u)] = 0x9Au; dice_put16(ram, 0x3000u, 0x0171u, 0x2Au); dice_put16(ram, 0x3000u, 0x0173u, 0x3D5u);
    ram[dice_physical(0x03D5u, 0x2Fu)] = 0xEAu; dice_put16(ram, 0x03D5u, 0x30u, 0x0350u); dice_put16(ram, 0x03D5u, 0x32u, 0x4000u);
    ram[dice_physical(0x03D5u, 0x2Au)] = 0xEAu; dice_put16(ram, 0x03D5u, 0x2Bu, 0x047Bu); dice_put16(ram, 0x03D5u, 0x2Du, 0x4000u);
    dice_site(ram, 0x3000u, 0x004Bu, 0x48u, 0x0DE3u);
    dice_site(ram, 0x3000u, 0x00B7u, 0x48u, 0x0DE3u);
    dice_site(ram, 0x3000u, 0x00C6u, 0x48u, 0x0DE3u);
    dice_system(ram, 0x1000u, seed);
    dice_put16(ram, 0x2000u, 0x8000u, 0x7000u);
    dice_put16(ram, 0x2000u, 0x8002u, 0x7001u);
    return cpu;
}

/* Steps of seed := seed * 8088405h + 1 from `from` to `to`, or 0 when not within `limit`. */
static unsigned steps_between(uint32_t from, uint32_t to, unsigned limit)
{
    unsigned n;
    for (n = 1; n <= limit; ++n) {
        from = from * 0x08088405u + 1u;
        if (from == to)
            return n;
    }
    return 0;
}

int main(void)
{
    uint8_t *ram = malloc(DICE_RAM);
    pr_u33_0810_cpu cpu;
    pr_u33_0810_callee callee;
    unsigned written, i, n;
    CHECK(ram != NULL);

    /* No eligible cell: the 13x5 sweep stamps the grid, the roll draws nothing, the frame unwinds. */
    cpu = enter(ram, 0u, 0x12345678u);
    CHECK(pr_u33_neighbourhood_0810(ram, DICE_RAM, &cpu, &callee) == PR_U33_0810_OK);
    CHECK(cpu.ip == 0x7000u && cpu.cs == 0x7001u && cpu.sp == 0x8004u && cpu.bp == 0x7777u);
    CHECK(dice_seed(ram, 0x1000u) == 0x12345678u);
    for (written = 0, i = 0; i < 50u * 25u; ++i)
        written += ram[dice_physical(0x6000u, (uint16_t)(0x0107u + i))] != 0u;
    CHECK(written > 100u);
    CHECK(ram[dice_physical(0x2000u, 0x7FFDu)] == 6u && ram[dice_physical(0x2000u, 0x7FFCu)] == 2u);   /* dx, dy at exit */

    /* Every cell eligible: the roll visits all 1,250 cells, so the generator advanced 1,250..3,750 steps. */
    cpu = enter(ram, 1u, 0x0BADF00Du);
    CHECK(pr_u33_neighbourhood_0810(ram, DICE_RAM, &cpu, &callee) == PR_U33_0810_OK);
    n = steps_between(0x0BADF00Du, dice_seed(ram, 0x1000u), 3750u);
    CHECK(n >= 1250u && n <= 3750u);

    /* A dice site that is not the loaded original: CODE from the final +0000 call, after the sweep. */
    cpu = enter(ram, 1u, 1u);
    ram[dice_physical(0x3000u, 0x004Bu)] = 0xCDu;
    CHECK(pr_u33_neighbourhood_0810(ram, DICE_RAM, &cpu, &callee) == PR_U33_0810_CODE);
    CHECK(callee.site == 0x8Du && dice_seed(ram, 0x1000u) == 1u);

    /* A +012B site that is not the loaded original. With the party at x = 6 the first position (x 0, y 1)
     * is on the lookup grid, so the first +01AD call reaches the site and reports CODE. */
    cpu = enter(ram, 0u, 1u);
    ram[dice_physical(0x1000u, 0x6AADu)] = 6u;
    ram[dice_physical(0x3000u, 0x0155u)] = 0xCDu;
    CHECK(pr_u33_neighbourhood_0810(ram, DICE_RAM, &cpu, &callee) == PR_U33_0810_CODE && callee.site == 0x42u);
    /* With the party at x = 5 the first position is off the lookup grid; the corrupted site is first met
     * inside +0615's own +01AD probe, which +0810 reports as CALLEE at its +088D call. */
    cpu = enter(ram, 0u, 1u);
    ram[dice_physical(0x3000u, 0x0155u)] = 0xCDu;
    CHECK(pr_u33_neighbourhood_0810(ram, DICE_RAM, &cpu, &callee) == PR_U33_0810_CALLEE && callee.site == 0x7Du);

    {
        pr_u33_0810_cpu before = enter(ram, 0u, 1u), after = before;
        CHECK(pr_u33_neighbourhood_0810(NULL, DICE_RAM, &after, NULL) == PR_U33_0810_ARGUMENT);
        CHECK(pr_u33_neighbourhood_0810(ram, DICE_RAM - 1u, &after, NULL) == PR_U33_0810_ARGUMENT);
        CHECK(pr_u33_neighbourhood_0810(ram, DICE_RAM, NULL, NULL) == PR_U33_0810_ARGUMENT);
        CHECK(memcmp(&before, &after, sizeof before) == 0);
    }
    free(ram);
    printf("u33 +0810: sweep, roll, dice-site, lookup-site and guard cases passed\n");
    return 0;
}
