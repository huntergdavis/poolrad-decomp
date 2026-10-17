#include "poolrad/recovered/overlay_u6c_record_tier_3fb3.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U6C_3FB3_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U6C_3FB3_RAM_SIZE - 1u)] = (uint8_t)(value >> 8u);
}

static pr_u6c_3fb3_cpu prepare(uint8_t *ram, uint8_t gate, uint8_t table)
{
    pr_u6c_3fb3_cpu cpu = {
        0xABCDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0xF000u, 0x3FB3u, 0x1000u, 0x1111u,
        0x2000u, 0x0ED7u
    };
    memset(ram, 0xA5, PR_U6C_3FB3_RAM_SIZE);
    memset(ram + physical(0x4000u, 0x1096u), gate, 8);
    ram[physical(0x4000u, 0x1014u)] = 18;
    ram[physical(0x4000u, 0x102Fu)] = 2;
    ram[physical(cpu.ds, 0x40A0u)] = table;
    put16(ram, cpu.ss, 0x8000u, 0x0712u);
    put16(ram, cpu.ss, 0x8002u, 0xF000u);
    put16(ram, cpu.ss, 0x8004u, 0x1000u);
    put16(ram, cpu.ss, 0x8006u, 0x4000u);
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(PR_U6C_3FB3_RAM_SIZE);
    pr_u6c_3fb3_cpu cpu;
    assert(ram);
    cpu = prepare(ram, 4, 3); /* (34 + 40) / 8 = 9, remainder 2. */
    assert(pr_u6c_record_tier_3fb3(NULL, PR_U6C_3FB3_RAM_SIZE, &cpu) == PR_U6C_3FB3_ARGUMENT);
    assert(pr_u6c_record_tier_3fb3(ram, PR_U6C_3FB3_RAM_SIZE, NULL) == PR_U6C_3FB3_ARGUMENT);
    assert(pr_u6c_record_tier_3fb3(ram, 1, &cpu) == PR_U6C_3FB3_ARGUMENT);
    assert(cpu.sp == 0x8000u && ram[physical(cpu.ss, 0x7FFEu)] == 0xA5u);
    assert(pr_u6c_record_tier_3fb3(ram, PR_U6C_3FB3_RAM_SIZE, &cpu) == PR_U6C_3FB3_OK);
    assert(cpu.ax == 9 && cpu.cx == 8 && cpu.dx == 2);
    assert(cpu.es == 0x4000u && cpu.di == 0x1000u);
    assert(cpu.bp == 0x7777u && cpu.sp == 0x8008u && cpu.cs == 0xF000u && cpu.ip == 0x0712u);
    assert(cpu.bx == 0x3456u && cpu.si == 0x9ABCu && cpu.ds == 0x1000u && cpu.ss == 0x2000u);
    assert((cpu.flags & (uint16_t)~0x08D5u) == 0x0602u);
    assert(ram[physical(cpu.ss, 0x7FFCu)] == 8 && ram[physical(cpu.ss, 0x7FFBu)] == 40);
    assert(ram[physical(cpu.ss, 0x7FFAu)] == 9 && ram[physical(cpu.ss, 0x7FFDu)] == 9);
    assert(ram[physical(cpu.ss, 0x7FF4u)] == 0xA5u);

    cpu = prepare(ram, 1, 14); /* callee 128 -> -128, clamp to 1. */
    assert(pr_u6c_record_tier_3fb3(ram, PR_U6C_3FB3_RAM_SIZE, &cpu) == PR_U6C_3FB3_OK);
    assert(cpu.ax == 1 && cpu.cx == 8 && cpu.dx == 136 && cpu.flags == 0x0687u);
    cpu = prepare(ram, 30, 29); /* (242 - 8) / 8 = 29, remainder 2. */
    assert(pr_u6c_record_tier_3fb3(ram, PR_U6C_3FB3_RAM_SIZE, &cpu) == PR_U6C_3FB3_OK);
    assert(cpu.ax == 29 && cpu.cx == 8 && cpu.dx == 2);

    cpu = prepare(ram, 0, 3);
    assert(pr_u6c_record_tier_3fb3(ram, PR_U6C_3FB3_RAM_SIZE, &cpu) == PR_U6C_3FB3_DIVIDE_ERROR);
    assert(cpu.ip == 0x4078u && cpu.cs == 0xF000u && cpu.bp == 0x7FFEu && cpu.sp == 0x7FF4u);
    assert(cpu.ax == 0 && cpu.cx == 0 && cpu.dx == 0 && cpu.flags == 0x0646u);
    cpu = prepare(ram, 4, 3);
    cpu.sp = 18;
    put16(ram, cpu.ss, 18, 0x0712u);
    put16(ram, cpu.ss, 20, 0xF000u);
    put16(ram, cpu.ss, 22, 0x1000u);
    put16(ram, cpu.ss, 24, 0x4000u);
    assert(pr_u6c_record_tier_3fb3(ram, PR_U6C_3FB3_RAM_SIZE, &cpu) == PR_U6C_3FB3_TRANSFER);
    assert(cpu.ip == 0x401Bu && cpu.cs == 0xA5A5u && cpu.sp == 6 && cpu.bp == 16);
    free(ram);
    return 0;
}
