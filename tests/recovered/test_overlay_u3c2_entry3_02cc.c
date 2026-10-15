#include "poolrad/recovered/overlay_u3c2_entry3_02cc.h"

#include <assert.h>
#include <stdlib.h>

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U3C2_02CC_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = at(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U3C2_02CC_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = at(seg, off);
    return (uint16_t)(ram[a] |
        ((uint16_t)ram[(a + 1u) & (PR_U3C2_02CC_RAM_SIZE - 1u)] << 8));
}

static void run_case(unsigned which)
{
    uint8_t *ram = calloc(PR_U3C2_02CC_RAM_SIZE, 1u);
    pr_u3c2_02cc_cpu cpu = {0};
    const uint16_t rec = 0x2200u;
    assert(ram != NULL);
    cpu.ax = 0xBEEFu;
    cpu.bp = 0x2468u;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x02CCu;
    cpu.ds = 0x6000u;
    cpu.ss = 0x7000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.ss, cpu.sp, 0x1234u);
    put16(ram, cpu.ss, 0x8002u, 0xD000u);
    put16(ram, cpu.ss, 0x8004u, rec);
    put16(ram, cpu.ss, 0x8006u, 0x5000u);
    ram[at(cpu.ds, 0x25C0u + 15u)] = 0xA5u;
    ram[at(cpu.ds, 0x25C1u)] = 0xB6u;
    ram[at(cpu.ds, 0x25C4u)] = 0xC7u;
    put16(ram, 0x5000u, rec + 0x0Au, which == 1u ? 2u : 5u);
    put16(ram, 0x5000u, rec + 0x0Cu, which == 1u ? 3u : 3u);
    put16(ram, 0x5000u, rec + 0x14u, which == 2u ? 2u : 1u);
    put16(ram, 0x5000u, rec + 4u, 2u);
    put16(ram, 0x5000u, rec + 0x16u, 3u);
    put16(ram, 0x5000u, rec + 6u, 4u);
    put16(ram, 0x5000u, rec + 8u, which == 1u ? 0xFFFFu : 0u);
    put16(ram, 0x5000u, rec + 0x0Eu, 7u);
    put16(ram, 0x5000u, rec + 0x10u, 9u);
    ram[at(0x5000u, rec + 0x12u)] = 2u;
    ram[at(0x5000u, rec + 0x13u)] = which == 1u ? 0xFFu : 3u;
    assert(pr_u3c2_entry3_02cc(ram, PR_U3C2_02CC_RAM_SIZE, &cpu) ==
           PR_U3C2_02CC_OK);
    assert(cpu.bp == 0x2468u && cpu.sp == 0x8008u);
    assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u);
    if (which == 0u) {
        assert(get16(ram, 0x5000u, rec + 8u) == 7u);
        assert(get16(ram, 0x5000u, rec + 0x14u) == 3u);
        assert(get16(ram, 0x5000u, rec + 0x16u) == 6u);
        assert(ram[at(0x5000u, rec + 0x19u)] == 0xA5u);
        assert((uint8_t)cpu.ax == 1u);
    } else if (which == 1u) {
        assert(get16(ram, 0x5000u, rec + 8u) == 8u);
        assert(get16(ram, 0x5000u, rec + 0x16u) == 2u);
        assert(ram[at(0x5000u, rec + 0x19u)] == 0xB6u);
        assert((uint8_t)cpu.ax == 1u);
    } else {
        assert(get16(ram, 0x5000u, rec + 8u) == 0u);
        assert(ram[at(0x5000u, rec + 0x19u)] == 0xC7u);
        assert((uint8_t)cpu.ax == 0u);
    }
    free(ram);
}

int main(void)
{
    uint8_t *ram = calloc(PR_U3C2_02CC_RAM_SIZE, 1u);
    pr_u3c2_02cc_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ip = 0x02CCu;
    assert(pr_u3c2_entry3_02cc(NULL, PR_U3C2_02CC_RAM_SIZE, &cpu) ==
           PR_U3C2_02CC_ARGUMENT);
    assert(pr_u3c2_entry3_02cc(ram, 1u, &cpu) == PR_U3C2_02CC_ARGUMENT);
    free(ram);
    run_case(0u);
    run_case(1u);
    run_case(2u);
    return 0;
}
