#include "poolrad/recovered/overlay_u86_record_predicate_323d.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U86_323D_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U86_323D_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static pr_u86_323d_cpu prepare(uint8_t *ram, uint8_t index, uint8_t value)
{
    pr_u86_323d_cpu cpu = {
        0xABCDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0xF000u, 0x323Du, 0x1000u, 0x1111u,
        0x2000u, 0x0202u
    };
    memset(ram, 0xA5, PR_U86_323D_RAM_SIZE);
    put16(ram, cpu.ss, 0x8000u, 0x0712u);
    put16(ram, cpu.ss, 0x8002u, cpu.cs);
    put16(ram, cpu.ss, 0x8004u, 0x1000u);
    put16(ram, cpu.ss, 0x8006u, 0x4000u);
    ram[physical(0x4000u, 0x102Eu)] = index;
    ram[physical(cpu.ds, (uint16_t)(0x5582u + 16u * index))] = value;
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(PR_U86_323D_RAM_SIZE);
    pr_u86_323d_cpu cpu;
    unsigned value;
    assert(ram);
    cpu = prepare(ram, 5, 12);
    assert(pr_u86_record_predicate_323d(NULL, PR_U86_323D_RAM_SIZE, &cpu) == PR_U86_323D_ARGUMENT);
    assert(pr_u86_record_predicate_323d(ram, PR_U86_323D_RAM_SIZE, NULL) == PR_U86_323D_ARGUMENT);
    assert(pr_u86_record_predicate_323d(ram, 1, &cpu) == PR_U86_323D_ARGUMENT);
    for (value = 9; value <= 15; ++value) {
        cpu = prepare(ram, 5, (uint8_t)value);
        assert(pr_u86_record_predicate_323d(ram, PR_U86_323D_RAM_SIZE, &cpu) == PR_U86_323D_OK);
        assert(cpu.ax == (uint16_t)(value >= 11 && value <= 13));
        assert(cpu.cx == 0x5604u && cpu.di == 0x50u && cpu.es == 0x4000u);
        assert(cpu.bx == 0x3456u && cpu.dx == 0x789Au && cpu.si == 0x9ABCu);
        assert(cpu.bp == 0x7777u && cpu.sp == 0x8008u && cpu.ip == 0x0712u && cpu.cs == 0xF000u);
        if (value == 12) assert(cpu.flags == 0x0293u); /* defined CMP AF */
    }
    cpu = prepare(ram, 5, 12);
    put16(ram, cpu.ss, 0x8004u, 0);
    put16(ram, cpu.ss, 0x8006u, 0);
    assert(pr_u86_record_predicate_323d(ram, PR_U86_323D_RAM_SIZE, &cpu) == PR_U86_323D_OK);
    assert(cpu.ax == 0 && cpu.cx == 0x5678u && cpu.di == 0xBCDEu && cpu.es == 0x1111u);
    assert((cpu.flags & 0xFFEFu) == 0x0246u);

    cpu = prepare(ram, 255, 13);
    put16(ram, cpu.ss, 0x8004u, 0xFFDEu);
    ram[physical(0x4000u, 0x000Cu)] = 255;
    assert(pr_u86_record_predicate_323d(ram, PR_U86_323D_RAM_SIZE, &cpu) == PR_U86_323D_OK);
    assert(cpu.ax == 1 && cpu.di == 0x0FF0u);

    cpu = prepare(ram, 0, 12);
    cpu.ss = cpu.ds;
    cpu.sp = 0x5584u; /* saved BP at table[0] changes 12 to 77 */
    put16(ram, cpu.ss, 0x5584u, 0x0712u);
    put16(ram, cpu.ss, 0x5586u, 0xF000u);
    put16(ram, cpu.ss, 0x5588u, 0x1000u);
    put16(ram, cpu.ss, 0x558Au, 0x4000u);
    assert(pr_u86_record_predicate_323d(ram, PR_U86_323D_RAM_SIZE, &cpu) == PR_U86_323D_OK);
    assert(cpu.ax == 0 && ram[physical(cpu.ds, 0x5582u)] == 0x77u);
    free(ram);
    return 0;
}
