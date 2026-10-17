#include "poolrad/recovered/overlay_u33_cell_lookup_012b.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "u33 +012B line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define RAM PR_U33_012B_RAM_SIZE

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

/* Loaded-state code: the two far-call sites in the body at 3000:012B and the
 * stubs at 03D5:002A/002F pointing at the lookups loaded at 4000:047B/0350. */
static void code(uint8_t *ram)
{
    static const uint8_t field_site[5] = {0x9Au, 0x2Fu, 0x00u, 0xD5u, 0x03u};
    static const uint8_t nibble_site[5] = {0x9Au, 0x2Au, 0x00u, 0xD5u, 0x03u};
    static const uint8_t field_stub[5] = {0xEAu, 0x50u, 0x03u, 0x00u, 0x40u};
    static const uint8_t nibble_stub[5] = {0xEAu, 0x7Bu, 0x04u, 0x00u, 0x40u};
    memcpy(ram + physical(0x3000u, 0x0155u), field_site, 5u);
    memcpy(ram + physical(0x3000u, 0x0170u), nibble_site, 5u);
    memcpy(ram + physical(0x03D5u, 0x002Fu), field_stub, 5u);
    memcpy(ram + physical(0x03D5u, 0x002Au), nibble_stub, 5u);
}

static pr_u33_012b_cpu frame(uint8_t *ram, uint8_t x, uint8_t y, uint8_t facing)
{
    pr_u33_012b_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
                           0x7777u, 0x8000u, 0x3000u, 0x012Bu, 0x1000u, 0x1111u,
                           0x2000u, 0x7202u};
    memset(ram, 0xA5, RAM);
    code(ram);
    ram[physical(0x1000u, 0x84DCu)] = 5u;      /* map id */
    ram[physical(0x1000u, 0x6AAEu)] = 4u;      /* party y */
    put16(ram, 0x1000u, 0x6A5Cu, 0x0100u);     /* far buffer pointer 5000:0100 */
    put16(ram, 0x1000u, 0x6A5Eu, 0x5000u);
    memset(ram + physical(0x5000u, 0x0100u), 0, 0x400u);
    put16(ram, cpu.ss, cpu.sp, 0x7000u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0x7001u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u), 0x7777u);   /* the caller's BP, unused */
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 6u), facing);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 8u), y);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 10u), x);
    return cpu;
}

static void check_return(const pr_u33_012b_cpu *cpu, uint8_t result, uint16_t flags)
{
    CHECK((uint8_t)cpu->ax == result && cpu->flags == flags);
    CHECK(cpu->bx == 0x3456u && cpu->si == 0x9ABCu);
    CHECK(cpu->bp == 0x7777u && cpu->sp == 0x800Cu);
    CHECK(cpu->ip == 0x7000u && cpu->cs == 0x7001u);
    CHECK(cpu->ds == 0x1000u && cpu->ss == 0x2000u);
}

int main(void)
{
    uint8_t *ram = malloc(RAM);
    pr_u33_012b_cpu cpu;
    CHECK(ram != NULL);

    /* In the grid, selector 2, nibble 5 and a zero two-bit field: entry 3
     * returns 0 and the body returns 1 with the OR AL,AL flags of zero. */
    cpu = frame(ram, 3u, 4u, 2u);
    ram[physical(0x5000u, 0x0100u + 16u * 4u + 3u)] = 0x05u;
    CHECK(pr_u33_cell_lookup_012b(ram, RAM, &cpu) == PR_U33_012B_OK);
    check_return(&cpu, 1u, 0x7246u);
    CHECK(cpu.es == 0x5000u && cpu.cx == 0x0002u);   /* ES:DI and CX from the selector-2 field shift */

    /* A zero nibble leaves entry 3 at its default 1, so the nibble entry runs and returns 0. */
    cpu = frame(ram, 3u, 4u, 2u);
    ram[physical(0x5000u, 0x0100u + 0x300u + 16u * 4u + 3u)] = 0x04u;
    CHECK(pr_u33_cell_lookup_012b(ram, RAM, &cpu) == PR_U33_012B_OK);
    check_return(&cpu, 0u, 0x7246u);
    CHECK(cpu.cx == 0x5604u);                          /* only CL := 4 inside the nibble entry */

    /* Field nonzero and nibble 5: returns 3 with the OR flags of 5. */
    cpu = frame(ram, 3u, 4u, 2u);
    ram[physical(0x5000u, 0x0100u + 0x300u + 16u * 4u + 3u)] = 0x04u;
    ram[physical(0x5000u, 0x0100u + 16u * 4u + 3u)] = 0x05u;
    CHECK(pr_u33_cell_lookup_012b(ram, RAM, &cpu) == PR_U33_012B_OK);
    check_return(&cpu, 3u, 0x7206u);                   /* OR AL,AL of 3: even parity */
    CHECK(ram[physical(0x2000u, 0x7FFDu)] == 3u);   /* [BP-1] */

    /* Out of the grid on the party row: facing 2 returns 0, facing 1 returns 1. */
    cpu = frame(ram, 16u, 4u, 2u);
    CHECK(pr_u33_cell_lookup_012b(ram, RAM, &cpu) == PR_U33_012B_OK);
    check_return(&cpu, 0u, 0x7246u);
    CHECK(cpu.es == 0x1111u && cpu.di == 0xBCDEu);   /* no lookup ran */
    cpu = frame(ram, 16u, 4u, 1u);
    CHECK(pr_u33_cell_lookup_012b(ram, RAM, &cpu) == PR_U33_012B_OK);
    check_return(&cpu, 1u, 0x7293u);                   /* CMP 1,6: CF AF SF */
    cpu = frame(ram, 0x80u, 9u, 6u);
    CHECK(pr_u33_cell_lookup_012b(ram, RAM, &cpu) == PR_U33_012B_OK);
    check_return(&cpu, 1u, 0x7206u);                   /* CMP 9,4: result 5, even parity */

    /* A stub that is not a loaded far jump stops before any effect beyond the frame. */
    cpu = frame(ram, 3u, 4u, 2u);
    ram[physical(0x03D5u, 0x002Fu)] = 0xCDu;
    CHECK(pr_u33_cell_lookup_012b(ram, RAM, &cpu) == PR_U33_012B_CODE);

    /* Host guards leave the state untouched. */
    {
        pr_u33_012b_cpu before = frame(ram, 1u, 1u, 0u), after = before;
        CHECK(pr_u33_cell_lookup_012b(NULL, RAM, &after) == PR_U33_012B_ARGUMENT);
        CHECK(pr_u33_cell_lookup_012b(ram, RAM - 1u, &after) == PR_U33_012B_ARGUMENT);
        CHECK(pr_u33_cell_lookup_012b(ram, RAM, NULL) == PR_U33_012B_ARGUMENT);
        CHECK(memcmp(&before, &after, sizeof before) == 0);
    }
    free(ram);
    printf("u33 +012B: field, nibble, party-row, stub and guard cases passed\n");
    return 0;
}
