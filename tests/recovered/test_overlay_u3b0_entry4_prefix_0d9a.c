#include "poolrad/recovered/overlay_u3b0_entry4_prefix_0d9a.h"

#include <assert.h>
#include <stdlib.h>

static size_t at(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) &
           (PR_U3B0_0D9A_RAM_SIZE - 1u);
}

static uint16_t get16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t a = at(segment, offset);
    return (uint16_t)(ram[a] |
                      ((uint16_t)ram[(a + 1u) &
                                      (PR_U3B0_0D9A_RAM_SIZE - 1u)] << 8));
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset,
                  uint16_t value)
{
    size_t a = at(segment, offset);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U3B0_0D9A_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static void run(uint16_t sp, uint16_t source_offset,
                uint16_t source_segment)
{
    uint8_t *ram = calloc(PR_U3B0_0D9A_RAM_SIZE, 1u);
    pr_u3b0_0d9a_cpu cpu = {0};
    uint16_t bp = (uint16_t)(sp - 2u);
    uint16_t final_sp = (uint16_t)(bp - 0x0198u);
    assert(ram != NULL);
    cpu.ax = 0xABCDu;
    cpu.bx = 0x1357u;
    cpu.di = 0x579Bu;
    cpu.bp = 0x68ACu;
    cpu.sp = sp;
    cpu.cs = 0xF000u;
    cpu.ip = 0x0D9Au;
    cpu.ds = 0x6000u;
    cpu.es = 0x8000u;
    cpu.ss = 0x7000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.ss, (uint16_t)(bp + 0x2Au), source_offset);
    put16(ram, cpu.ss, (uint16_t)(bp + 0x2Cu), source_segment);
    assert(pr_u3b0_entry4_prefix_0d9a(ram, PR_U3B0_0D9A_RAM_SIZE,
                                        &cpu) == PR_U3B0_0D9A_OK);
    assert(cpu.ip == 0x0DAFu && cpu.cs == 0xF000u);
    assert(cpu.bp == bp && cpu.sp == final_sp);
    assert(cpu.ax == 0x0028u && cpu.di == (uint16_t)(bp - 0x2Au));
    assert(cpu.es == source_segment && cpu.ds == 0x6000u);
    assert(cpu.bx == 0x1357u);
    assert(get16(ram, cpu.ss, bp) == 0x68ACu);
    assert(get16(ram, cpu.ss, final_sp) == 0x0028u);
    assert(get16(ram, cpu.ss, (uint16_t)(final_sp + 2u)) ==
           (uint16_t)(bp - 0x2Au));
    assert(get16(ram, cpu.ss, (uint16_t)(final_sp + 4u)) == cpu.ss);
    assert(get16(ram, cpu.ss, (uint16_t)(final_sp + 6u)) == source_offset);
    assert(get16(ram, cpu.ss, (uint16_t)(final_sp + 8u)) == source_segment);
    if (sp == 0x8000u) assert(cpu.flags == 0x0202u);
    if (sp == 0x0000u) assert(cpu.flags == 0x0282u);
    free(ram);
}

int main(void)
{
    uint8_t *ram = calloc(PR_U3B0_0D9A_RAM_SIZE, 1u);
    pr_u3b0_0d9a_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ip = 0x0D9Au;
    assert(pr_u3b0_entry4_prefix_0d9a(NULL, PR_U3B0_0D9A_RAM_SIZE,
                                        &cpu) == PR_U3B0_0D9A_ARGUMENT);
    assert(pr_u3b0_entry4_prefix_0d9a(ram, PR_U3B0_0D9A_RAM_SIZE - 1u,
                                        &cpu) == PR_U3B0_0D9A_ARGUMENT);
    cpu.ip = 0x0D9Bu;
    assert(pr_u3b0_entry4_prefix_0d9a(ram, PR_U3B0_0D9A_RAM_SIZE,
                                        &cpu) == PR_U3B0_0D9A_ARGUMENT);
    free(ram);
    run(0x8000u, 0x1000u, 0x5000u);
    run(0x0000u, 0xFFFFu, 0x0000u);
    run(0xFFFFu, 0x0000u, 0x9000u);
    return 0;
}
