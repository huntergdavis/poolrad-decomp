#include "poolrad/recovered/overlay_u38_clear_cycle_2e08.h"

#include <assert.h>
#include <stdlib.h>

#define RAM_SIZE PR_U38_101D_RAM_SIZE

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (RAM_SIZE - 1u);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = at(seg, off);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) &
                                      (RAM_SIZE - 1u)] << 8));
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off,
                  uint16_t value)
{
    size_t a = at(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static void run(uint8_t latch, uint8_t gate)
{
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u38_2e08_cpu cpu = {0};
    uint8_t expected_latch = latch;
    uint8_t expected_al = latch;
    assert(ram != NULL);
    cpu.ax = 0xA5CDu;
    cpu.bp = 0x7777u;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x2E08u;
    cpu.ds = 0x3000u;
    cpu.ss = 0xE000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.ss, cpu.sp, 0x1234u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    ram[at(cpu.ds, 0x6816u)] = 0x12u;
    ram[at(cpu.ds, 0x6817u)] = latch;
    ram[at(cpu.ds, 0x6818u)] = 0xA7u;
    ram[at(cpu.ds, 0x682Au)] = gate;
    assert(pr_u38_clear_cycle_2e08(ram, RAM_SIZE, &cpu) == PR_U38_2E08_OK);
    assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u);
    assert(cpu.sp == 0x800Eu && cpu.bp == 0x7777u);
    if (latch == 0x0Bu || latch == 0x35u ||
        latch == 0x34u || latch == 0x37u) {
        expected_latch = 0u;
        expected_al = latch == 0x37u ? latch : 0u;
    }
    assert(ram[at(cpu.ds, 0x6817u)] == expected_latch);
    assert(ram[at(cpu.ds, 0x6818u)] ==
           (expected_latch == 0u && latch != 0u ? 0u : 0xA7u));
    assert(ram[at(cpu.ds, 0x6816u)] == (gate == 0u ? 0x64u : 0x12u));
    assert(cpu.ax == (uint16_t)(0xA500u | expected_al));
    assert((cpu.flags & 0x40u) == (gate == 0u ? 0x40u : 0u));
    assert(get16(ram, cpu.ss, 0x7FF8u) == 0x2E27u);
    assert(get16(ram, cpu.ss, 0x7FFAu) == 0xF000u);
    assert(get16(ram, cpu.ss, 0x7FFCu) == 0xA537u);
    free(ram);
}

int main(void)
{
    for (unsigned latch = 0u; latch <= 255u; ++latch) {
        run((uint8_t)latch, 0u);
        run((uint8_t)latch, 0x80u);
    }
    return 0;
}
