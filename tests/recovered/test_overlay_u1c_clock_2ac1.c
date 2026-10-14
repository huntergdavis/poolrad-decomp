#include "poolrad/recovered/overlay_u1c_clock_2ac1.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned calls[3];

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U1C_2AC1_RAM_SIZE - 1u);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = physical(seg, off);
    return (uint16_t)(ram[a] |
                      ((uint16_t)ram[(a + 1u) & (PR_U1C_2AC1_RAM_SIZE - 1u)] << 8));
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = physical(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U1C_2AC1_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static int external(void *context, uint8_t *ram, size_t size,
                    pr_u1c_2ac1_cpu *cpu, pr_u1c_2ac1_call call)
{
    (void)context;
    assert(size == PR_U1C_2AC1_RAM_SIZE);
    ++calls[call];
    if (call == PR_U1C_2AC1_LOADER) {
        assert(cpu->ip == 0x2ACAu);
        assert(get16(ram, cpu->ss, cpu->sp) == 0xA501u);
        assert(cpu->flags == 0x7206u);
        ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1u))] = 7u;
        cpu->ax = 0xB100u;
    } else if (call == PR_U1C_2AC1_VALUE) {
        assert(cpu->ip == 0x2AD3u);
        assert(get16(ram, cpu->ss, cpu->sp) == 0xB107u);
        cpu->ax = 0xC912u;
    } else {
        assert(cpu->ip == 0x2AE2u);
        assert(get16(ram, cpu->ss, cpu->sp) == 0xC901u);
        assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) == 0xC912u);
        ram[physical(cpu->ds, 0x84E8u)] = 0x5Au;
    }
    return 1;
}

int main(void)
{
    uint8_t *ram = calloc(PR_U1C_2AC1_RAM_SIZE, 1u);
    pr_u1c_2ac1_cpu cpu = {
        0xA5CDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0xF000u, 0x2AC1u, 0x3000u, 0x1111u,
        0xE000u, 0x72D3u
    };
    assert(ram != NULL);
    assert(pr_u1c_clock_2ac1(NULL, PR_U1C_2AC1_RAM_SIZE,
                               &cpu, external, NULL) == PR_U1C_2AC1_ARGUMENT);
    put16(ram, cpu.ss, cpu.sp, 0x0712u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    ram[physical(cpu.ss, 0x7FFDu)] = 0x42u;
    assert(pr_u1c_clock_2ac1(ram, PR_U1C_2AC1_RAM_SIZE,
                               &cpu, external, NULL) == PR_U1C_2AC1_OK);
    assert(calls[0] == 1u && calls[1] == 1u && calls[2] == 1u);
    assert(ram[physical(cpu.ss, 0x7FFDu)] == 0x12u);
    assert(ram[physical(cpu.ds, 0x84E8u)] == 0x5Au);
    assert(cpu.sp == 0x8004u && cpu.bp == 0x7777u &&
           cpu.cs == 0xD000u && cpu.ip == 0x0712u);
    free(ram);
    puts("unit 0x1C +2AC1 clock tests passed");
    return 0;
}
