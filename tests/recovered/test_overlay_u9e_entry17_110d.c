#include "poolrad/recovered/overlay_u9e_entry17_110d.h"

#include <assert.h>
#include <stdlib.h>

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U9E_110D_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = at(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U9E_110D_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static void run(uint8_t source, uint16_t expected_ax,
                uint8_t expected_a, uint8_t expected_b, int alias)
{
    uint8_t *ram = calloc(PR_U9E_110D_RAM_SIZE, 1u);
    pr_u9e_110d_cpu cpu = {0};
    uint16_t a_off = 0x2200u, b_off = alias ? a_off : 0x2300u;
    assert(ram != NULL);
    cpu.ax = 0xBEEFu;
    cpu.bp = 0x2468u;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x110Du;
    cpu.ss = 0x7000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.ss, 0x8000u, 0x1234u);
    put16(ram, cpu.ss, 0x8002u, 0xD000u);
    put16(ram, cpu.ss, 0x8004u, a_off);
    put16(ram, cpu.ss, 0x8006u, 0x5000u);
    put16(ram, cpu.ss, 0x8008u, b_off);
    put16(ram, cpu.ss, 0x800Au, 0x5000u);
    put16(ram, cpu.ss, 0x800Cu, 0x2400u);
    put16(ram, cpu.ss, 0x800Eu, 0x5000u);
    ram[at(0x5000u, 0x2403u)] = source;
    assert(pr_u9e_entry17_110d(ram, PR_U9E_110D_RAM_SIZE, &cpu) ==
           PR_U9E_110D_OK);
    assert(cpu.ax == expected_ax);
    assert(ram[at(0x5000u, a_off)] == expected_a);
    assert(ram[at(0x5000u, b_off)] == expected_b);
    assert(cpu.bp == 0x2468u && cpu.sp == 0x8010u);
    assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u);
    free(ram);
}

int main(void)
{
    uint8_t *ram = calloc(PR_U9E_110D_RAM_SIZE, 1u);
    pr_u9e_110d_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ip = 0x110Du;
    assert(pr_u9e_entry17_110d(NULL, PR_U9E_110D_RAM_SIZE, &cpu) ==
           PR_U9E_110D_ARGUMENT);
    assert(pr_u9e_entry17_110d(ram, PR_U9E_110D_RAM_SIZE - 1u, &cpu) ==
           PR_U9E_110D_ARGUMENT);
    free(ram);
    run(0x40u, 0x003Fu, 0x3Fu, 0x12u, 0);
    run(0xE7u, 0x0003u, 0x00u, 0x03u, 0);
    run(0x40u, 0x003Fu, 0x12u, 0x12u, 1);
    return 0;
}
