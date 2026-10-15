#include "poolrad/recovered/overlay_u3b0_entry4_node_loop_0e30.h"

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

static void frame(uint8_t *ram, pr_u3b0_0dea_cpu *cpu, uint8_t flag,
                  uint16_t next_offset, uint16_t next_segment)
{
    *cpu = (pr_u3b0_0dea_cpu){0};
    cpu->bp = 0x7FFEu;
    cpu->ss = 0x7000u;
    cpu->ip = 0x0E30u;
    cpu->flags = 0x0AD7u;
    put16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x89u), 0x2200u);
    put16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x87u), 0x5000u);
    put16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x8Bu), 0u);
    ram[at(0x5000u, 0x2229u)] = flag;
    put16(ram, 0x5000u, 0x222Au, next_offset);
    put16(ram, 0x5000u, 0x222Cu, next_segment);
}

int main(void)
{
    uint8_t *ram = calloc(PR_U3B0_0DEA_RAM_SIZE, 1u);
    pr_u3b0_0dea_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ip = 0x0E30u;
    assert(pr_u3b0_entry4_node_loop_0e30(NULL, PR_U3B0_0DEA_RAM_SIZE,
                                           &cpu, 2u) == PR_U3B0_0E30_ARGUMENT);
    assert(pr_u3b0_entry4_node_loop_0e30(ram, PR_U3B0_0DEA_RAM_SIZE - 1u,
                                           &cpu, 2u) == PR_U3B0_0E30_ARGUMENT);
    assert(pr_u3b0_entry4_node_loop_0e30(ram, PR_U3B0_0DEA_RAM_SIZE,
                                           &cpu, 0u) == PR_U3B0_0E30_ARGUMENT);

    frame(ram, &cpu, 0u, 0u, 0u);
    assert(pr_u3b0_entry4_node_loop_0e30(ram, PR_U3B0_0DEA_RAM_SIZE,
                                           &cpu, 2u) == PR_U3B0_0E30_OK);
    assert(cpu.ip == 0x0E5Fu && cpu.ax == 0x7200u);
    assert(cpu.di == 0x2200u && cpu.es == 0x5000u);
    assert(get16(ram, cpu.ss, (uint16_t)(cpu.bp - 0x8Bu)) == 0u);
    assert(cpu.flags == 0x0246u);

    frame(ram, &cpu, 1u, 0u, 0u);
    assert(pr_u3b0_entry4_node_loop_0e30(ram, PR_U3B0_0DEA_RAM_SIZE,
                                           &cpu, 1u) == PR_U3B0_0E30_OK);
    assert(cpu.ip == 0x0E5Fu && cpu.ax == 0u && cpu.dx == 0u);
    assert(get16(ram, cpu.ss, (uint16_t)(cpu.bp - 0x8Bu)) == 1u);
    assert(cpu.flags == 0x0246u);

    frame(ram, &cpu, 1u, 0x2200u, 0x5000u); /* deliberate cycle */
    assert(pr_u3b0_entry4_node_loop_0e30(ram, PR_U3B0_0DEA_RAM_SIZE,
                                           &cpu, 1u) == PR_U3B0_0E30_LIMIT);
    assert(cpu.ip == 0x0E45u);
    assert(get16(ram, cpu.ss, (uint16_t)(cpu.bp - 0x8Bu)) == 1u);
    free(ram);
    return 0;
}
