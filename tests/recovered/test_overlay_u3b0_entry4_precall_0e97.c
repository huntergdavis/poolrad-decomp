#include "poolrad/recovered/overlay_u3b0_entry4_precall_0e97.h"

#include <assert.h>
#include <stdlib.h>

static size_t at(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) &
           (PR_U3B0_0DEA_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset,
                  uint16_t value)
{
    size_t p = at(segment, offset);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U3B0_0DEA_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t p = at(segment, offset);
    return (uint16_t)(ram[p] |
                      ((uint16_t)ram[(p + 1u) &
                                     (PR_U3B0_0DEA_RAM_SIZE - 1u)] << 8));
}

static void frame(uint8_t *ram, pr_u3b0_0dea_cpu *cpu,
                  uint16_t global, uint16_t value, uint16_t count)
{
    *cpu = (pr_u3b0_0dea_cpu){0};
    cpu->bp = 0x7FFEu;
    cpu->sp = 0x7E70u;
    cpu->ss = 0x7000u;
    cpu->ds = 0x6000u;
    cpu->cs = 0xF000u;
    cpu->ip = 0x0E97u;
    cpu->flags = 0x0246u;
    put16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Au), 0x2200u);
    put16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Cu), 0x5000u);
    put16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Eu), 0x2100u);
    put16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x10u), 0x6000u);
    put16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x8Du), count);
    put16(ram, cpu->ds, 0x6B29u, global);
    put16(ram, 0x5000u, 0x2200u, value);
    ram[at(0x6000u, 0x2100u)] = 0u;
}

int main(void)
{
    uint8_t *ram = calloc(PR_U3B0_0DEA_RAM_SIZE, 1u);
    pr_u3b0_0dea_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ip = 0x0E97u;
    assert(pr_u3b0_entry4_precall_0e97(NULL, PR_U3B0_0DEA_RAM_SIZE,
                                         &cpu) == PR_U3B0_0E97_ARGUMENT);
    assert(pr_u3b0_entry4_precall_0e97(ram, PR_U3B0_0DEA_RAM_SIZE - 1u,
                                         &cpu) == PR_U3B0_0E97_ARGUMENT);

    frame(ram, &cpu, 0u, 0u, 1u);
    assert(pr_u3b0_entry4_precall_0e97(ram, PR_U3B0_0DEA_RAM_SIZE,
                                         &cpu) == PR_U3B0_0E97_OK);
    assert(cpu.ip == 0x0ED2u && cpu.sp == 0x7E6Au);
    assert(get16(ram, 0x5000u, 0x2200u) == 1u);
    assert(get16(ram, cpu.ds, 0x6B29u) == 0u);
    assert(ram[at(0x6000u, 0x2100u)] == 0u);
    assert(get16(ram, cpu.ss, cpu.sp) == 0xF000u);
    assert(get16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u)) == 0x7FFEu);
    assert(get16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u)) == 0u);

    frame(ram, &cpu, 5u, 2u, 1u);
    assert(pr_u3b0_entry4_precall_0e97(ram, PR_U3B0_0DEA_RAM_SIZE,
                                         &cpu) == PR_U3B0_0E97_OK);
    assert(get16(ram, 0x5000u, 0x2200u) == 3u);
    assert(get16(ram, cpu.ds, 0x6B29u) == 0u);
    assert(ram[at(0x6000u, 0x2100u)] == 1u);
    assert(cpu.di == 0x2200u && cpu.es == 0x5000u);
    free(ram);
    return 0;
}
