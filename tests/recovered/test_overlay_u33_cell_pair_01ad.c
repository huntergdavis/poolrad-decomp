#include "poolrad/recovered/overlay_u33_cell_pair_01ad.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "u33 +01AD line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define RAM PR_U33_01AD_RAM_SIZE

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (RAM - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (RAM - 1u)] = (uint8_t)(value >> 8);
}

static pr_u33_01ad_cpu frame(uint8_t *ram, uint8_t x, uint8_t y, uint8_t facing)
{
    static const uint8_t field_site[5] = {0x9Au, 0x2Fu, 0x00u, 0xD5u, 0x03u};
    static const uint8_t nibble_site[5] = {0x9Au, 0x2Au, 0x00u, 0xD5u, 0x03u};
    static const uint8_t field_stub[5] = {0xEAu, 0x50u, 0x03u, 0x00u, 0x40u};
    static const uint8_t nibble_stub[5] = {0xEAu, 0x7Bu, 0x04u, 0x00u, 0x40u};
    pr_u33_01ad_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
                           0x7777u, 0x8000u, 0x3000u, 0x01ADu, 0x1000u, 0x1111u,
                           0x2000u, 0x7202u};
    memset(ram, 0xA5, RAM);
    memcpy(ram + physical(0x3000u, 0x0155u), field_site, 5u);   /* +012B loaded at 3000:012B */
    memcpy(ram + physical(0x3000u, 0x0170u), nibble_site, 5u);
    memcpy(ram + physical(0x03D5u, 0x002Fu), field_stub, 5u);
    memcpy(ram + physical(0x03D5u, 0x002Au), nibble_stub, 5u);
    ram[physical(0x1000u, 0x84DCu)] = 5u;
    ram[physical(0x1000u, 0x6AAEu)] = 4u;
    put16(ram, 0x1000u, 0x6A5Cu, 0x0100u);
    put16(ram, 0x1000u, 0x6A5Eu, 0x5000u);
    memset(ram + physical(0x5000u, 0x0100u), 0, 0x400u);
    memset(ram + physical(0x1000u, 0x2736u), 0, 0x10Au);        /* step tables: facing 2 steps x by +1 */
    ram[physical(0x1000u, 0x2736u + 2u)] = 1u;
    ram[physical(0x1000u, 0x273Fu + 2u)] = 0u;
    put16(ram, cpu.ss, cpu.sp, 0x7000u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0x7001u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u), facing);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 6u), y);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 8u), x);
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(RAM);
    pr_u33_01ad_cpu cpu;
    CHECK(ram != NULL);

    /* Cell (3,4) facing 2: nibble 5 and a zero field give 1; cell (4,4) facing 6:
     * an all-zero buffer keeps entry 3 at its default 1 and the nibble is 0: OR = 1. */
    cpu = frame(ram, 3u, 4u, 2u);
    ram[physical(0x5000u, 0x0100u + 16u * 4u + 3u)] = 0x05u;
    CHECK(pr_u33_cell_pair_01ad(ram, RAM, &cpu) == PR_U33_01AD_OK);
    CHECK((uint8_t)cpu.ax == 1u && cpu.flags == 0x7202u);
    CHECK(ram[physical(0x2000u, 0x7FFAu)] == 6u);     /* [BP-4]: (2 + 4) mod 8 */
    CHECK(ram[physical(0x2000u, 0x7FFCu)] == 1u && ram[physical(0x2000u, 0x7FFBu)] == 0u);
    CHECK(cpu.bx == 0x3456u && cpu.si == 0x9ABCu && cpu.bp == 0x7777u && cpu.sp == 0x800Au);
    CHECK(cpu.ip == 0x7000u && cpu.cs == 0x7001u && cpu.es == 0x5000u);

    /* Both cells return 3: the OR is 3 with odd parity. */
    cpu = frame(ram, 3u, 4u, 2u);
    ram[physical(0x5000u, 0x0100u + 0x300u + 16u * 4u + 3u)] = 0x04u;
    ram[physical(0x5000u, 0x0100u + 16u * 4u + 3u)] = 0x05u;
    ram[physical(0x5000u, 0x0100u + 0x300u + 16u * 4u + 4u)] = 0xC0u;
    ram[physical(0x5000u, 0x0100u + 0x100u + 16u * 4u + 4u)] = 0x0Fu;
    CHECK(pr_u33_cell_pair_01ad(ram, RAM, &cpu) == PR_U33_01AD_OK);
    CHECK((uint8_t)cpu.ax == 3u && cpu.flags == 0x7206u);   /* OR of 3: even parity */

    /* Facing 5 steps nowhere here; (5 + 4) mod 8 = 1; both cells off the grid return 1. */
    cpu = frame(ram, 0xFFu, 4u, 5u);
    CHECK(pr_u33_cell_pair_01ad(ram, RAM, &cpu) == PR_U33_01AD_OK);
    CHECK((uint8_t)cpu.ax == 1u && ram[physical(0x2000u, 0x7FFAu)] == 1u);
    CHECK(cpu.es == 0x1111u && cpu.di == 0x0005u);   /* no lookup ran; DI holds the last table index */

    /* A corrupted stub inside +012B surfaces as CODE. */
    cpu = frame(ram, 3u, 4u, 2u);
    ram[physical(0x03D5u, 0x002Fu)] = 0u;
    CHECK(pr_u33_cell_pair_01ad(ram, RAM, &cpu) == PR_U33_01AD_CODE);

    {
        pr_u33_01ad_cpu before = frame(ram, 1u, 1u, 0u), after = before;
        CHECK(pr_u33_cell_pair_01ad(NULL, RAM, &after) == PR_U33_01AD_ARGUMENT);
        CHECK(pr_u33_cell_pair_01ad(ram, RAM - 1u, &after) == PR_U33_01AD_ARGUMENT);
        CHECK(pr_u33_cell_pair_01ad(ram, RAM, NULL) == PR_U33_01AD_ARGUMENT);
        CHECK(memcmp(&before, &after, sizeof before) == 0);
    }
    free(ram);
    printf("u33 +01AD: pair, both-three, off-grid, stub and guard cases passed\n");
    return 0;
}
