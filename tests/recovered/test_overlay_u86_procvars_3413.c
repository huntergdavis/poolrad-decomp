#include "poolrad/recovered/overlay_u86_procvars_3413.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U86_3413_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U86_3413_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t word(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & (PR_U86_3413_RAM_SIZE - 1u)] << 8));
}

static pr_u86_3413_cpu prepare(uint8_t *ram)
{
    unsigned i;
    pr_u86_3413_cpu cpu = {
        0xABCDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0xF000u, 0x3413u, 0x1000u, 0x1111u,
        0x2000u, 0x0ED7u
    };
    memset(ram, 0xA5, PR_U86_3413_RAM_SIZE);
    put16(ram, cpu.cs, 0x341Au, 0x2096u);
    for (i = 0; i < 67u; ++i)
        put16(ram, cpu.cs, (uint16_t)(0x343Eu + 13u * i), (uint16_t)(0x3000u + i));
    put16(ram, cpu.cs, 0x37A5u, 0x4096u);
    put16(ram, cpu.ss, cpu.sp, 0x0712u);
    put16(ram, cpu.ss, 0x8002u, cpu.cs);
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(PR_U86_3413_RAM_SIZE);
    pr_u86_3413_cpu cpu;
    assert(ram);
    cpu = prepare(ram);
    assert(pr_u86_procvars_3413(NULL, PR_U86_3413_RAM_SIZE, &cpu) == PR_U86_3413_ARGUMENT);
    assert(pr_u86_procvars_3413(ram, PR_U86_3413_RAM_SIZE, NULL) == PR_U86_3413_ARGUMENT);
    assert(pr_u86_procvars_3413(ram, 1, &cpu) == PR_U86_3413_ARGUMENT);
    assert(pr_u86_procvars_3413(ram, PR_U86_3413_RAM_SIZE, &cpu) == PR_U86_3413_OK);
    assert(word(ram, cpu.ds, 0x6B84u) == 0x0034u && word(ram, cpu.ds, 0x6B86u) == 0x2096u);
    assert(word(ram, cpu.ds, 0x6B88u) == 0x0057u && word(ram, cpu.ds, 0x6B8Au) == 0x3000u);
    assert(word(ram, cpu.ds, 0x6C90u) == 0x0156u && word(ram, cpu.ds, 0x6C92u) == 0x3042u);
    assert(word(ram, cpu.ds, 0x6988u) == 0x015Bu && word(ram, cpu.ds, 0x698Au) == 0x4096u);
    assert(ram[physical(cpu.ds, 0x681Fu)] == 0 && ram[physical(cpu.ds, 0x6DBFu)] == 0);
    assert(word(ram, cpu.ds, 0x6DB5u) == 0 && word(ram, cpu.ds, 0x6DB7u) == 0);
    assert(ram[physical(cpu.ds, 0x6821u)] == 1 && ram[physical(cpu.ds, 0x6820u)] == 0xA5u);
    assert(cpu.ax == 0x015Bu && cpu.dx == 0x4096u && cpu.bp == 0x7777u);
    assert(cpu.sp == 0x8004u && cpu.cs == 0xF000u && cpu.ip == 0x0712u);
    assert(cpu.flags == 0x0646u && cpu.bx == 0x3456u && cpu.cx == 0x5678u);
    assert(cpu.si == 0x9ABCu && cpu.di == 0xBCDEu && cpu.es == 0x1111u);

    cpu = prepare(ram);
    cpu.ss = cpu.ds;
    cpu.sp = 0x6B86u;
    put16(ram, cpu.ss, cpu.sp, 0x0712u);
    put16(ram, cpu.ss, 0x6B88u, 0xF000u);
    assert(pr_u86_procvars_3413(ram, PR_U86_3413_RAM_SIZE, &cpu) == PR_U86_3413_OK);
    assert(cpu.bp == 0x0034u && cpu.ip == 0x2096u && cpu.cs == 0x0057u);
    assert(cpu.sp == 0x6B8Au);
    free(ram);
    return 0;
}
