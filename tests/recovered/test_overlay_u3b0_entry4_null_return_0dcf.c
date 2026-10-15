#include "poolrad/recovered/overlay_u3b0_entry4_null_return_0dcf.h"

#include <assert.h>
#include <stdlib.h>

static size_t at(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) &
           (PR_U3B0_0DC7_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset,
                  uint16_t value)
{
    size_t p = at(segment, offset);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U3B0_0DC7_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

int main(void)
{
    uint8_t *ram = calloc(PR_U3B0_0DC7_RAM_SIZE, 1u);
    pr_u3b0_0dc7_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ip = 0x0DCFu;
    assert(pr_u3b0_entry4_null_return_0dcf(NULL, PR_U3B0_0DC7_RAM_SIZE,
                                             &cpu) == PR_U3B0_0DCF_ARGUMENT);
    assert(pr_u3b0_entry4_null_return_0dcf(ram, PR_U3B0_0DC7_RAM_SIZE - 1u,
                                             &cpu) == PR_U3B0_0DCF_ARGUMENT);
    cpu.ip = 0x0DD0u;
    assert(pr_u3b0_entry4_null_return_0dcf(ram, PR_U3B0_0DC7_RAM_SIZE,
                                             &cpu) == PR_U3B0_0DCF_ARGUMENT);

    cpu.ip = 0x0DCFu;
    cpu.bp = 0x7FFEu;
    cpu.sp = 0x7E70u;
    cpu.ss = 0x7000u;
    cpu.cs = 0xF000u;
    cpu.ax = 0xABCDu;
    cpu.flags = 0x0AD7u;
    put16(ram, cpu.ss, cpu.bp, 0x4321u);
    put16(ram, cpu.ss, (uint16_t)(cpu.bp + 2u), 0x1234u);
    put16(ram, cpu.ss, (uint16_t)(cpu.bp + 4u), 0xD000u);
    put16(ram, cpu.ss, (uint16_t)(cpu.bp + 6u), 0x2100u);
    put16(ram, cpu.ss, (uint16_t)(cpu.bp + 8u), 0x9000u);
    put16(ram, cpu.ss, (uint16_t)(cpu.bp + 0x0Au), 0x2000u);
    put16(ram, cpu.ss, (uint16_t)(cpu.bp + 0x0Cu), 0x9000u);
    put16(ram, 0x9000u, 0x2000u, 0xBEEFu);
    put16(ram, 0x9000u, 0x2100u, 0xBEEFu);
    put16(ram, 0x9000u, 0x2102u, 0xBEEFu);
    ram[at(cpu.ss, (uint16_t)(cpu.bp - 1u))] = 0x7Fu;
    assert(pr_u3b0_entry4_null_return_0dcf(ram, PR_U3B0_0DC7_RAM_SIZE,
                                             &cpu) == PR_U3B0_0DCF_OK);
    assert(ram[at(0x9000u, 0x2000u)] == 0u);
    assert(ram[at(0x9000u, 0x2001u)] == 0u);
    assert(ram[at(0x9000u, 0x2100u)] == 0u);
    assert(ram[at(0x9000u, 0x2103u)] == 0u);
    assert(ram[at(cpu.ss, 0x7FFDu)] == 0u);
    assert(cpu.ax == 0u && cpu.di == 0x2100u && cpu.es == 0x9000u);
    assert(cpu.bp == 0x4321u && cpu.sp == 0x802Cu);
    assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u);
    assert(cpu.flags == 0x0246u);
    free(ram);
    return 0;
}
