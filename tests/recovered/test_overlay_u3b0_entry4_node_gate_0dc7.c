#include "poolrad/recovered/overlay_u3b0_entry4_node_gate_0dc7.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t at(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) &
           (PR_U3B0_0DC7_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset,
                  uint16_t value)
{
    size_t a = at(segment, offset);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U3B0_0DC7_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static void run(uint16_t offset, uint16_t segment, uint16_t expected_flags)
{
    uint8_t *ram = calloc(PR_U3B0_0DC7_RAM_SIZE, 1u);
    uint8_t *before = malloc(PR_U3B0_0DC7_RAM_SIZE);
    pr_u3b0_0dc7_cpu cpu = {0};
    assert(ram != NULL && before != NULL);
    cpu.ax = 0xABCDu;
    cpu.bx = 0x1357u;
    cpu.bp = 0x7FFEu;
    cpu.sp = 0x7E70u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x0DC7u;
    cpu.ds = 0x6000u;
    cpu.es = 0x7000u;
    cpu.ss = 0x7000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.ss, (uint16_t)(cpu.bp + 0x14u), offset);
    put16(ram, cpu.ss, (uint16_t)(cpu.bp + 0x16u), segment);
    memcpy(before, ram, PR_U3B0_0DC7_RAM_SIZE);
    assert(pr_u3b0_entry4_node_gate_0dc7(ram, PR_U3B0_0DC7_RAM_SIZE,
                                           &cpu) == PR_U3B0_0DC7_OK);
    assert(cpu.ax == (uint16_t)(offset | segment));
    assert(cpu.ip == (cpu.ax != 0u ? 0x0DEAu : 0x0DCFu));
    assert(cpu.flags == expected_flags);
    assert(cpu.sp == 0x7E70u && cpu.bp == 0x7FFEu);
    assert(cpu.bx == 0x1357u && cpu.ds == 0x6000u && cpu.es == 0x7000u);
    assert(memcmp(ram, before, PR_U3B0_0DC7_RAM_SIZE) == 0);
    free(before);
    free(ram);
}

int main(void)
{
    uint8_t *ram = calloc(PR_U3B0_0DC7_RAM_SIZE, 1u);
    pr_u3b0_0dc7_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ip = 0x0DC7u;
    assert(pr_u3b0_entry4_node_gate_0dc7(NULL, PR_U3B0_0DC7_RAM_SIZE,
                                           &cpu) == PR_U3B0_0DC7_ARGUMENT);
    assert(pr_u3b0_entry4_node_gate_0dc7(ram, PR_U3B0_0DC7_RAM_SIZE - 1u,
                                           &cpu) == PR_U3B0_0DC7_ARGUMENT);
    cpu.ip = 0x0DC8u;
    assert(pr_u3b0_entry4_node_gate_0dc7(ram, PR_U3B0_0DC7_RAM_SIZE,
                                           &cpu) == PR_U3B0_0DC7_ARGUMENT);
    free(ram);
    run(0u, 0u, 0x0246u);
    run(0x2200u, 0u, 0x0206u);
    run(0u, 0x8000u, 0x0286u);
    run(0x8001u, 0u, 0x0282u);
    return 0;
}
