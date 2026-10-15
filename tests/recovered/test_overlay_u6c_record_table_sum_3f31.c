#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U6C_3F31_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U6C_3F31_RAM_SIZE - 1u)] = (uint8_t)(value >> 8u);
}

static pr_u6c_3f31_cpu prepare(uint8_t *ram, uint8_t type, uint8_t kind, uint8_t table)
{
    pr_u6c_3f31_cpu cpu = {
        0xABCDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0xF000u, 0x3F31u, 0x1000u, 0x1111u,
        0x2000u, 0x0ED7u
    };
    memset(ram, 0xA5, PR_U6C_3F31_RAM_SIZE);
    memset(ram + physical(0x4000u, 0x1096u), 1, 8);
    ram[physical(0x4000u, 0x1014u)] = type;
    ram[physical(0x4000u, 0x102Fu)] = kind;
    ram[physical(cpu.ds, (uint16_t)(0x408Eu + type))] = table;
    put16(ram, cpu.ss, 0x8000u, 0x0712u);
    put16(ram, cpu.ss, 0x8002u, 0xF000u);
    put16(ram, cpu.ss, 0x8004u, 0x1000u);
    put16(ram, cpu.ss, 0x8006u, 0x4000u);
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(PR_U6C_3F31_RAM_SIZE);
    static const uint8_t gates[8] = {0,1,127,128,255,0,128,255};
    pr_u6c_3f31_cpu cpu;
    unsigned i;
    assert(ram);
    cpu = prepare(ram, 18, 2, 3);
    assert(pr_u6c_record_table_sum_3f31(NULL, PR_U6C_3F31_RAM_SIZE, &cpu) == PR_U6C_3F31_ARGUMENT);
    assert(pr_u6c_record_table_sum_3f31(ram, PR_U6C_3F31_RAM_SIZE, NULL) == PR_U6C_3F31_ARGUMENT);
    assert(pr_u6c_record_table_sum_3f31(ram, 1, &cpu) == PR_U6C_3F31_ARGUMENT);
    assert(cpu.sp == 0x8000u && ram[physical(cpu.ss, 0x7FFEu)] == 0xA5u);
    assert(pr_u6c_record_table_sum_3f31(ram, PR_U6C_3F31_RAM_SIZE, &cpu) == PR_U6C_3F31_OK);
    assert(cpu.ax == 40 && cpu.di == 0x1000u && cpu.es == 0x4000u && cpu.flags == 0x0646u);
    assert(cpu.bp == 0x7777u && cpu.sp == 0x8008u && cpu.cs == 0xF000u && cpu.ip == 0x0712u);
    assert(cpu.bx == 0x3456u && cpu.cx == 0x5678u && cpu.dx == 0x789Au && cpu.si == 0x9ABCu);
    assert(cpu.ds == 0x1000u && cpu.ss == 0x2000u);
    assert(ram[physical(cpu.ss, 0x7FFDu)] == 40 && ram[physical(cpu.ss, 0x7FFCu)] == 40);
    assert(ram[physical(cpu.ss, 0x7FFBu)] == 7 && ram[physical(cpu.ss, 0x7FFAu)] == 0xA5u);
    assert(ram[physical(0x4000u, 0x1096u)] == 1);
    for (i = 16; i <= 18; ++i) {
        cpu = prepare(ram, (uint8_t)i, 2, 3);
        assert(pr_u6c_record_table_sum_3f31(ram, PR_U6C_3F31_RAM_SIZE, &cpu) == PR_U6C_3F31_OK);
        assert(cpu.ax == 8u * (3u + i - 16u));
    }
    cpu = prepare(ram, 255, 3, 3);
    assert(pr_u6c_record_table_sum_3f31(ram, PR_U6C_3F31_RAM_SIZE, &cpu) == PR_U6C_3F31_OK);
    assert(cpu.ax == 24);
    cpu = prepare(ram, 255, 2, 255);
    assert(pr_u6c_record_table_sum_3f31(ram, PR_U6C_3F31_RAM_SIZE, &cpu) == PR_U6C_3F31_OK);
    assert(cpu.ax == 8);
    cpu = prepare(ram, 18, 2, 3);
    for (i = 0; i < 8; ++i) ram[physical(0x4000u, (uint16_t)(0x1096u+i))] = gates[i];
    assert(pr_u6c_record_table_sum_3f31(ram, PR_U6C_3F31_RAM_SIZE, &cpu) == PR_U6C_3F31_OK);
    assert(cpu.ax == 10 && cpu.di == 0x1007u);
    /* DS:408E aliases the zero-initialized total, so each lookup sees it. */
    cpu = prepare(ram, 0, 0, 99);
    cpu.ds = cpu.ss;
    cpu.sp = 0x4092u;
    put16(ram, cpu.ss, 0x4092u, 0x0712u);
    put16(ram, cpu.ss, 0x4094u, 0xF000u);
    put16(ram, cpu.ss, 0x4096u, 0x1000u);
    put16(ram, cpu.ss, 0x4098u, 0x4000u);
    assert(pr_u6c_record_table_sum_3f31(ram, PR_U6C_3F31_RAM_SIZE, &cpu) == PR_U6C_3F31_OK);
    assert(cpu.ax == 0 && cpu.sp == 0x409Au && cpu.flags == 0x0646u);
    free(ram);
    return 0;
}
