#include "poolrad/recovered/overlay_u3b0_entry4_loop_setup_0e0c.h"

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

int main(void)
{
    uint8_t *ram = calloc(PR_U3B0_0DEA_RAM_SIZE, 1u);
    pr_u3b0_0dea_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ip = 0x0E0Cu;
    assert(pr_u3b0_entry4_loop_setup_0e0c(NULL, PR_U3B0_0DEA_RAM_SIZE,
                                            &cpu) == PR_U3B0_0E0C_ARGUMENT);
    assert(pr_u3b0_entry4_loop_setup_0e0c(ram, PR_U3B0_0DEA_RAM_SIZE - 1u,
                                            &cpu) == PR_U3B0_0E0C_ARGUMENT);
    cpu.ip = 0x0E0Du;
    assert(pr_u3b0_entry4_loop_setup_0e0c(ram, PR_U3B0_0DEA_RAM_SIZE,
                                            &cpu) == PR_U3B0_0E0C_ARGUMENT);

    cpu.ip = 0x0E0Cu;
    cpu.bp = 0x7FFEu;
    cpu.sp = 0x7E70u;
    cpu.ss = 0x7000u;
    cpu.ax = 0xABCDu;
    cpu.flags = 0x0AD7u;
    ram[at(cpu.ss, (uint16_t)(cpu.bp + 0x1Cu))] = 0xFEu; /* -2 */
    ram[at(cpu.ss, (uint16_t)(cpu.bp + 0x18u))] = 0x7Fu; /* +127 */
    put16(ram, cpu.ss, (uint16_t)(cpu.bp + 0x14u), 0x2200u);
    put16(ram, cpu.ss, (uint16_t)(cpu.bp + 0x16u), 0x5000u);
    assert(pr_u3b0_entry4_loop_setup_0e0c(ram, PR_U3B0_0DEA_RAM_SIZE,
                                            &cpu) == PR_U3B0_0E0C_OK);
    assert(get16(ram, cpu.ss, (uint16_t)(cpu.bp - 0x59u)) == 130u);
    assert(get16(ram, cpu.ss, (uint16_t)(cpu.bp - 0x89u)) == 0x2200u);
    assert(get16(ram, cpu.ss, (uint16_t)(cpu.bp - 0x87u)) == 0x5000u);
    assert(get16(ram, cpu.ss, (uint16_t)(cpu.bp - 0x8Bu)) == 0u);
    assert(cpu.ax == 0u && cpu.dx == 0x5000u && cpu.ip == 0x0E30u);
    assert(cpu.sp == 0x7E70u && cpu.bp == 0x7FFEu);
    assert(cpu.flags == 0x0246u);
    free(ram);
    return 0;
}
