#include "poolrad/recovered/overlay_u3b0_entry1_0000.h"

#include <assert.h>
#include <stdlib.h>

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U3B0_0000_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = at(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U3B0_0000_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static void run_case(uint16_t count, int cycle, uint16_t expected_off,
                     uint16_t expected_seg)
{
    uint8_t *ram = calloc(PR_U3B0_0000_RAM_SIZE, 1u);
    pr_u3b0_0000_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ax = 0xBEEFu;
    cpu.bp = 0x2468u;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0u;
    cpu.ss = 0x7000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.ss, 0x8000u, 0x1234u);
    put16(ram, cpu.ss, 0x8002u, 0xD000u);
    put16(ram, cpu.ss, 0x8004u, 0x2200u);
    put16(ram, cpu.ss, 0x8006u, 0x5000u);
    put16(ram, cpu.ss, 0x8008u, count);
    put16(ram, 0x5000u, 0x222Au, 0x2300u);
    put16(ram, 0x5000u, 0x222Cu, 0x5000u);
    put16(ram, 0x5000u, 0x232Au, 0x2400u);
    put16(ram, 0x5000u, 0x232Cu, 0x5000u);
    put16(ram, 0x5000u, 0x242Au, cycle ? 0x2300u : 0u);
    put16(ram, 0x5000u, 0x242Cu, cycle ? 0x5000u : 0u);
    assert(pr_u3b0_entry1_0000(ram, PR_U3B0_0000_RAM_SIZE, &cpu) ==
           PR_U3B0_0000_OK);
    assert(cpu.ax == expected_off && cpu.dx == expected_seg);
    assert(cpu.bp == 0x2468u && cpu.sp == 0x800Au);
    assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u);
    free(ram);
}

int main(void)
{
    uint8_t *ram = calloc(PR_U3B0_0000_RAM_SIZE, 1u);
    pr_u3b0_0000_cpu cpu = {0};
    assert(ram != NULL);
    assert(pr_u3b0_entry1_0000(NULL, PR_U3B0_0000_RAM_SIZE, &cpu) ==
           PR_U3B0_0000_ARGUMENT);
    assert(pr_u3b0_entry1_0000(ram, 1u, &cpu) == PR_U3B0_0000_ARGUMENT);
    free(ram);
    run_case(0u, 0, 0x2200u, 0x5000u);
    run_case(1u, 0, 0x2300u, 0x5000u);
    run_case(2u, 0, 0x2400u, 0x5000u);
    run_case(3u, 0, 0u, 0u);
    run_case(4u, 0, 0u, 0u);
    run_case(0xFFFFu, 0, 0u, 0u);
    run_case(3u, 1, 0x2300u, 0x5000u);
    return 0;
}
