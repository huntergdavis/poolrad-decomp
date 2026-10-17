#include "poolrad/recovered/overlay_u38_record_roll_2d38.h"
#include "dice_chain_fixture.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "u38 +2D38 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

static pr_u38_2d38_cpu frame(uint8_t *ram, int present, uint8_t record_byte, uint8_t sum, uint8_t flags, uint8_t count, uint32_t seed)
{
    pr_u38_2d38_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
                           0x7777u, 0x8000u, 0x3000u, 0x2D38u, 0x1000u, 0x1111u, 0x2000u, 0x7202u};
    unsigned i;
    memset(ram, 0xA5, DICE_RAM);
    dice_site(ram, 0x3000u, 0x2D6Au, 0x4Du, 0x0E2Eu);
    dice_system(ram, 0x1000u, seed);
    dice_put16(ram, 0x1000u, 0x5D92u, 0x0800u);        /* active record at 6000:0800 */
    dice_put16(ram, 0x1000u, 0x5D94u, 0x6000u);
    dice_put16(ram, 0x6000u, 0x08CCu, present ? 0x0900u : 0u);
    dice_put16(ram, 0x6000u, 0x08CEu, present ? 0x7000u : 0u);
    ram[dice_physical(0x7000u, 0x092Eu)] = record_byte;
    ram[dice_physical(0x1000u, 0x6818u)] = sum;
    ram[dice_physical(0x1000u, 0x6819u)] = flags;
    ram[dice_physical(0x1000u, 0x681Cu)] = count;
    dice_put16(ram, cpu.ss, cpu.sp, 0x7000u);
    dice_put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0x7001u);
    for (i = 0u; i < 5u; ++i)
        dice_put16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u + 2u * i), (uint16_t)(0x1111u * (i + 1u)));
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(DICE_RAM);
    pr_u38_2d38_cpu cpu;
    int chain = -1;
    uint32_t seed = 0xCAFEBABEu;
    uint8_t roll;
    CHECK(ram != NULL);

    /* Record byte 56h: 3d8 through entry 9 lands in 6818, entry 9 latches the count 3 at 681C,
     * and flag bit 3 adds that latch: 6818 = roll + 3. */
    roll = dice_roll(&seed, 3u, 8u);
    cpu = frame(ram, 1, 0x56u, 0x11u, 0x08u, 0x22u, 0xCAFEBABEu);
    CHECK(pr_u38_record_roll_2d38(ram, DICE_RAM, &cpu, &chain) == PR_U38_2D38_OK && chain == 0);
    CHECK(dice_seed(ram, 0x1000u) == seed);
    CHECK(ram[dice_physical(0x1000u, 0x681Cu)] == 3u);
    CHECK(ram[dice_physical(0x1000u, 0x6818u)] == (uint8_t)(roll + 3u));
    CHECK(cpu.bp == 0x7777u);                                           /* BX and SI follow the generator */
    CHECK(cpu.sp == 0x800Eu && cpu.ip == 0x7000u && cpu.cs == 0x7001u);

    /* Record byte 55h: no roll, the seed is untouched; flags 0 leave 6818 alone. */
    cpu = frame(ram, 1, 0x55u, 0x11u, 0x00u, 0x22u, 0x1u);
    CHECK(pr_u38_record_roll_2d38(ram, DICE_RAM, &cpu, &chain) == PR_U38_2D38_OK && chain == -1);
    CHECK(dice_seed(ram, 0x1000u) == 0x1u && ram[dice_physical(0x1000u, 0x6818u)] == 0x11u);
    CHECK(cpu.flags == 0x7246u);                       /* OR AL,AL of zero */
    CHECK(cpu.bx == 0x3456u && cpu.si == 0x9ABCu && cpu.cx == 0x5678u);   /* no generator ran */

    /* Null record pointer: no roll; flag bit 0 adds the existing latch with an 8-bit carry. */
    cpu = frame(ram, 0, 0x56u, 0xF0u, 0x01u, 0x20u, 0x1u);
    CHECK(pr_u38_record_roll_2d38(ram, DICE_RAM, &cpu, &chain) == PR_U38_2D38_OK);
    CHECK(ram[dice_physical(0x1000u, 0x6818u)] == 0x10u && (cpu.flags & 0x0001u) != 0u);

    /* A stub that is not a loaded far jump reports CODE before any roll. */
    cpu = frame(ram, 1, 0x56u, 0u, 0u, 0u, 0x1u);
    ram[dice_physical(DICE_STUB_PARAGRAPH, 0x4Du)] = 0xCDu;
    CHECK(pr_u38_record_roll_2d38(ram, DICE_RAM, &cpu, &chain) == PR_U38_2D38_CODE);

    {
        pr_u38_2d38_cpu before = frame(ram, 1, 0x56u, 0u, 0u, 0u, 0x1u), after = before;
        CHECK(pr_u38_record_roll_2d38(NULL, DICE_RAM, &after, NULL) == PR_U38_2D38_ARGUMENT);
        CHECK(pr_u38_record_roll_2d38(ram, DICE_RAM - 1u, &after, NULL) == PR_U38_2D38_ARGUMENT);
        CHECK(pr_u38_record_roll_2d38(ram, DICE_RAM, NULL, NULL) == PR_U38_2D38_ARGUMENT);
        CHECK(memcmp(&before, &after, sizeof before) == 0);
    }
    free(ram);
    printf("u38 +2D38: 3d8 latch, no-roll, null-record, stub and guard cases passed\n");
    return 0;
}
