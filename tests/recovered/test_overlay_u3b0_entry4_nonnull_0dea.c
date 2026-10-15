#include "poolrad/recovered/overlay_u3b0_entry4_nonnull_0dea.h"

#include <assert.h>
#include <stdlib.h>

static size_t at(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) &
           (PR_U3B0_0DEA_RAM_SIZE - 1u);
}

static void run(uint8_t first, uint8_t second, uint16_t expected_ax,
                uint16_t expected_dx, uint16_t expected_carry)
{
    uint8_t *ram = calloc(PR_U3B0_0DEA_RAM_SIZE, 1u);
    pr_u3b0_0dea_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ax = 0xABCDu;
    cpu.bx = 0x1357u;
    cpu.bp = 0x7FFEu;
    cpu.sp = 0x7E70u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x0DEAu;
    cpu.ds = 0x6000u;
    cpu.es = 0x7000u;
    cpu.ss = 0x7000u;
    cpu.flags = 0x0202u;
    ram[at(cpu.ss, (uint16_t)(cpu.bp + 0x1Eu))] = first;
    ram[at(cpu.ss, (uint16_t)(cpu.bp + 0x1Au))] = second;
    ram[at(cpu.ss, (uint16_t)(cpu.bp - 0x54u))] = 0xCCu;
    ram[at(cpu.ds, 0x6B2Bu)] = 0xA5u;
    assert(pr_u3b0_entry4_nonnull_0dea(ram, PR_U3B0_0DEA_RAM_SIZE,
                                         &cpu) == PR_U3B0_0DEA_OK);
    assert(cpu.ip == 0x0DFFu && cpu.cs == 0xF000u);
    assert(cpu.ax == expected_ax && cpu.dx == expected_dx);
    assert((cpu.flags & 1u) == expected_carry);
    assert(cpu.bp == 0x7FFEu && cpu.sp == 0x7E70u);
    assert(cpu.bx == 0x1357u && cpu.es == 0x7000u && cpu.ds == 0x6000u);
    assert(ram[at(cpu.ds, 0x6B2Bu)] == 1u);
    assert(ram[at(cpu.ss, (uint16_t)(cpu.bp - 0x54u))] ==
           (uint8_t)expected_ax);
    free(ram);
}

int main(void)
{
    uint8_t *ram = calloc(PR_U3B0_0DEA_RAM_SIZE, 1u);
    pr_u3b0_0dea_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ip = 0x0DEAu;
    assert(pr_u3b0_entry4_nonnull_0dea(NULL, PR_U3B0_0DEA_RAM_SIZE,
                                         &cpu) == PR_U3B0_0DEA_ARGUMENT);
    assert(pr_u3b0_entry4_nonnull_0dea(ram, PR_U3B0_0DEA_RAM_SIZE - 1u,
                                         &cpu) == PR_U3B0_0DEA_ARGUMENT);
    cpu.ip = 0x0DEBu;
    assert(pr_u3b0_entry4_nonnull_0dea(ram, PR_U3B0_0DEA_RAM_SIZE,
                                         &cpu) == PR_U3B0_0DEA_ARGUMENT);
    free(ram);
    run(5u, 2u, 0xFFFEu, 5u, 1u);
    run(0u, 0u, 1u, 0u, 0u);
    run(0xFEu, 0x7Fu, 0x0082u, 0xFFFEu, 1u);
    run(0x80u, 0x7Fu, 0x0100u, 0xFF80u, 1u);
    return 0;
}
