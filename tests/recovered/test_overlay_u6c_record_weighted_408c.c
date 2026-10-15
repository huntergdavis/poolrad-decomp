#include "poolrad/recovered/overlay_u6c_record_weighted_408c.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U6C_408C_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U6C_408C_RAM_SIZE - 1u)] = (uint8_t)(value >> 8u);
}

static pr_u6c_408c_cpu prepare(uint8_t *ram, uint8_t argument_gate,
                              uint8_t active_gate, uint8_t table)
{
    pr_u6c_408c_cpu cpu = {
        0xABCDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0xF000u, 0x408Cu, 0x1000u, 0x1111u,
        0x2000u, 0x0ED7u
    };
    memset(ram, 0xA5, PR_U6C_408C_RAM_SIZE);
    memset(ram + physical(0x4000u, 0x1096u), argument_gate, 8);
    memset(ram + physical(0x5000u, 0x2096u), active_gate, 8);
    ram[physical(0x5000u, 0x2014u)] = 18;
    ram[physical(0x5000u, 0x202Fu)] = 2;
    ram[physical(cpu.ds, 0x40A0u)] = table;
    put16(ram, cpu.ds, 0x5D92u, 0x2000u);
    put16(ram, cpu.ds, 0x5D94u, 0x5000u);
    put16(ram, cpu.ss, 0x8000u, 0x0712u);
    put16(ram, cpu.ss, 0x8002u, 0xF000u);
    put16(ram, cpu.ss, 0x8004u, 0x1000u);
    put16(ram, cpu.ss, 0x8006u, 0x4000u);
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(PR_U6C_408C_RAM_SIZE);
    pr_u6c_408c_cpu cpu;
    assert(ram);
    cpu = prepare(ram, 1, 1, 3); /* (70+40)/8 = 13, remainder 6. */
    assert(pr_u6c_record_weighted_408c(NULL, PR_U6C_408C_RAM_SIZE, &cpu) == PR_U6C_408C_ARGUMENT);
    assert(pr_u6c_record_weighted_408c(ram, PR_U6C_408C_RAM_SIZE, NULL) == PR_U6C_408C_ARGUMENT);
    assert(pr_u6c_record_weighted_408c(ram, 1, &cpu) == PR_U6C_408C_ARGUMENT);
    assert(cpu.sp == 0x8000u && ram[physical(cpu.ss, 0x7FFEu)] == 0xA5u);
    assert(pr_u6c_record_weighted_408c(ram, PR_U6C_408C_RAM_SIZE, &cpu) == PR_U6C_408C_OK);
    assert(cpu.ax == 13 && cpu.cx == 8 && cpu.dx == 6 && cpu.si == 2);
    assert(cpu.es == 0x5000u && cpu.di == 0x2000u && cpu.bx == 0x3456u);
    assert(cpu.bp == 0x7777u && cpu.sp == 0x8008u && cpu.cs == 0xF000u && cpu.ip == 0x0712u);
    assert(cpu.ds == 0x1000u && cpu.ss == 0x2000u && (cpu.flags & (uint16_t)~0x08D5u) == 0x0602u);
    cpu = prepare(ram, 1, 0, 3); /* Separate active record contributes zero. */
    assert(pr_u6c_record_weighted_408c(ram, PR_U6C_408C_RAM_SIZE, &cpu) == PR_U6C_408C_OK);
    assert(cpu.ax == 8 && cpu.dx == 6);
    cpu = prepare(ram, 5, 1, 3); /* Weighted 302 wraps to 46. */
    assert(pr_u6c_record_weighted_408c(ram, PR_U6C_408C_RAM_SIZE, &cpu) == PR_U6C_408C_OK);
    assert(cpu.ax == 10 && cpu.dx == 6 && cpu.si == 10);
    cpu = prepare(ram, 4, 1, 29); /* (244-8)/8 = 29, remainder 4. */
    assert(pr_u6c_record_weighted_408c(ram, PR_U6C_408C_RAM_SIZE, &cpu) == PR_U6C_408C_OK);
    assert(cpu.ax == 29 && cpu.dx == 4);
    cpu = prepare(ram, 0, 1, 13);
    ram[physical(0x4000u, 0x1098u)] = 127;
    assert(pr_u6c_record_weighted_408c(ram, PR_U6C_408C_RAM_SIZE, &cpu) == PR_U6C_408C_OK);
    assert(cpu.ax == 366 && cpu.cx == 1 && cpu.dx == 0 && cpu.si == 0x9ABCu);
    cpu = prepare(ram, 0, 1, 14); /* Zero count with -128 adjustment clamps. */
    assert(pr_u6c_record_weighted_408c(ram, PR_U6C_408C_RAM_SIZE, &cpu) == PR_U6C_408C_OK);
    assert(cpu.ax == 1 && cpu.cx == 0 && cpu.dx == 128 && cpu.flags == 0x0683u);
    cpu = prepare(ram, 0, 1, 3);
    assert(pr_u6c_record_weighted_408c(ram, PR_U6C_408C_RAM_SIZE, &cpu) == PR_U6C_408C_DIVIDE_ERROR);
    assert(cpu.ip == 0x4221u && cpu.ax == 40 && cpu.cx == 0 && cpu.dx == 0 && cpu.flags == 0x0606u);
    cpu = prepare(ram, 1, 1, 3);
    cpu.sp = 18;
    put16(ram, cpu.ss, 18, 0x0712u);
    put16(ram, cpu.ss, 20, 0xF000u);
    put16(ram, cpu.ss, 22, 0x1000u);
    put16(ram, cpu.ss, 24, 0x4000u);
    assert(pr_u6c_record_weighted_408c(ram, PR_U6C_408C_RAM_SIZE, &cpu) == PR_U6C_408C_TRANSFER);
    assert(cpu.ip == 0x41C4u && cpu.cs == 0xA5A5u && cpu.cx == 0x5603u && cpu.dx == 0 && cpu.si == 2);
    free(ram);
    return 0;
}
