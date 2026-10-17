#include "poolrad/recovered/overlay_u2f_party_pick_0131.h"
#include "dice_chain_fixture.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "u2F +0131 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

/* Nodes at 6000:k*120 link forward; each names a sub record at 7000:k*10 whose byte +3 is given. */
static pr_u2f_0131_cpu frame(uint8_t *ram, const uint8_t *subs, unsigned count, uint32_t seed)
{
    pr_u2f_0131_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
                           0x7777u, 0x8000u, 0x3000u, 0x0131u, 0x1000u, 0x1111u, 0x2000u, 0x7202u};
    unsigned k;
    memset(ram, 0xA5, DICE_RAM);
    dice_site(ram, 0x3000u, 0x015Du, 0x48u, 0x0DE3u);
    dice_system(ram, 0x1000u, seed);
    dice_put16(ram, 0x1000u, 0x5D96u, count ? 0u : 0u);
    dice_put16(ram, 0x1000u, 0x5D98u, count ? 0x6000u : 0u);
    for (k = 0u; k < count; ++k) {
        uint16_t node = (uint16_t)(k * 0x120u);
        dice_put16(ram, 0x6000u, (uint16_t)(node + 0x104u), k + 1u < count ? (uint16_t)(node + 0x120u) : 0u);
        dice_put16(ram, 0x6000u, (uint16_t)(node + 0x106u), k + 1u < count ? 0x6000u : 0u);
        dice_put16(ram, 0x6000u, (uint16_t)(node + 0x108u), (uint16_t)(k * 0x10u));
        dice_put16(ram, 0x6000u, (uint16_t)(node + 0x10Au), 0x7000u);
        ram[dice_physical(0x7000u, (uint16_t)(k * 0x10u + 3u))] = subs[k];
    }
    dice_put16(ram, cpu.ss, cpu.sp, 0x7000u);
    dice_put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0x7001u);
    dice_put16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u), 0x0100u);   /* result at 5000:0100 */
    dice_put16(ram, cpu.ss, (uint16_t)(cpu.sp + 6u), 0x5000u);
    return cpu;
}

static uint16_t result_offset(const uint8_t *ram)
{
    size_t at = dice_physical(0x5000u, 0x0100u);
    return (uint16_t)(ram[at] | ((uint16_t)ram[at + 1u] << 8));
}

int main(void)
{
    static const uint8_t subs[4] = {5u, 9u, 9u, 0x80u};
    uint8_t *ram = malloc(DICE_RAM);
    pr_u2f_0131_cpu cpu;
    int chain = -1;
    uint32_t seed = 0x12345678u;
    uint8_t rolls[4];
    unsigned k, expected = 1u;
    CHECK(ram != NULL);

    /* Four nodes: bytes 5, 9, 9 and -128. The first 9 is kept, and the second 9 replaces it only
     * when its roll is at least the kept roll; -128 never qualifies. Four d100 rolls are consumed. */
    for (k = 0u; k < 4u; ++k)
        rolls[k] = dice_roll(&seed, 1u, 100u);
    if (rolls[2] >= rolls[1])
        expected = 2u;
    cpu = frame(ram, subs, 4u, 0x12345678u);
    CHECK(pr_u2f_party_pick_0131(ram, DICE_RAM, &cpu, &chain) == PR_U2F_0131_OK && chain == 0);
    CHECK(dice_seed(ram, 0x1000u) == seed);
    CHECK(result_offset(ram) == (uint16_t)(expected * 0x120u));
    CHECK(ram[dice_physical(0x2000u, 0x7FFDu)] == 9u);                  /* [BP-1] best byte */
    CHECK(cpu.ax == 0u && cpu.dx == 0u);                                /* BX and SI follow the generator */
    CHECK(cpu.bp == 0x7777u && cpu.sp == 0x8008u && cpu.ip == 0x7000u && cpu.cs == 0x7001u);
    CHECK((cpu.flags & 0x08D5u) == 0x0000u || (cpu.flags & 0x0040u) == 0u); /* CMP 9,0: no ZF */

    /* Bytes all zero: nothing is kept and the result is cleared to 0:0 with XOR flags. */
    {
        static const uint8_t zeros[2] = {0u, 0u};
        cpu = frame(ram, zeros, 2u, 0x1u);
        dice_put16(ram, 0x5000u, 0x0100u, 0xBEEFu);
        dice_put16(ram, 0x5000u, 0x0102u, 0xBEEFu);
        CHECK(pr_u2f_party_pick_0131(ram, DICE_RAM, &cpu, &chain) == PR_U2F_0131_OK);
        CHECK(result_offset(ram) == 0u && ram[dice_physical(0x5000u, 0x0102u)] == 0u);
        CHECK(cpu.flags == 0x7246u && cpu.ax == 0u);
    }

    /* Empty list: no roll, the seed is untouched, the result is cleared. */
    cpu = frame(ram, subs, 0u, 0x55555555u);
    CHECK(pr_u2f_party_pick_0131(ram, DICE_RAM, &cpu, &chain) == PR_U2F_0131_OK && chain == -1);
    CHECK(dice_seed(ram, 0x1000u) == 0x55555555u && result_offset(ram) == 0u);
    CHECK(cpu.bx == 0x3456u && cpu.si == 0x9ABCu && cpu.cx == 0x5678u);   /* no generator ran */

    /* A stub that is not a loaded far jump reports CODE before any roll. */
    cpu = frame(ram, subs, 1u, 0x1u);
    ram[dice_physical(DICE_STUB_PARAGRAPH, 0x48u)] = 0xCDu;
    CHECK(pr_u2f_party_pick_0131(ram, DICE_RAM, &cpu, &chain) == PR_U2F_0131_CODE);
    CHECK(dice_seed(ram, 0x1000u) == 0x1u);

    {
        pr_u2f_0131_cpu before = frame(ram, subs, 1u, 0x1u), after = before;
        CHECK(pr_u2f_party_pick_0131(NULL, DICE_RAM, &after, NULL) == PR_U2F_0131_ARGUMENT);
        CHECK(pr_u2f_party_pick_0131(ram, DICE_RAM - 1u, &after, NULL) == PR_U2F_0131_ARGUMENT);
        CHECK(pr_u2f_party_pick_0131(ram, DICE_RAM, NULL, NULL) == PR_U2F_0131_ARGUMENT);
        CHECK(memcmp(&before, &after, sizeof before) == 0);
    }
    free(ram);
    printf("u2F +0131: four-node pick, zero bytes, empty list, stub and guard cases passed\n");
    return 0;
}
