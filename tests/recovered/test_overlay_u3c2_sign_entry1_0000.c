#include "poolrad/recovered/overlay_u3c2_sign_entry1_0000.h"

#include <assert.h>
#include <stdlib.h>

static size_t at(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) &
           (PR_U3C2_0000_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset,
                  uint16_t value)
{
    size_t p = at(segment, offset);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U3C2_0000_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t p = at(segment, offset);
    return (uint16_t)(ram[p] |
                      ((uint16_t)ram[(p + 1u) &
                                     (PR_U3C2_0000_RAM_SIZE - 1u)] << 8));
}

static void run(uint16_t value, uint16_t answer, uint16_t flags)
{
    uint8_t *ram = calloc(PR_U3C2_0000_RAM_SIZE, 1u);
    pr_u3c2_0000_cpu cpu = {0};
    assert(ram != NULL);
    cpu.bp = 0xBEEFu;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ss = 0x7000u;
    cpu.ax = 0xA55Au;
    cpu.flags = 0x0AD7u;
    put16(ram, cpu.ss, 0x8000u, 0x1234u);
    put16(ram, cpu.ss, 0x8002u, 0xD000u);
    put16(ram, cpu.ss, 0x8004u, value);
    assert(pr_u3c2_sign_entry1_0000(ram, PR_U3C2_0000_RAM_SIZE,
                                     &cpu) == PR_U3C2_0000_OK);
    assert(cpu.ax == answer && cpu.flags == flags);
    assert(cpu.bp == 0xBEEFu && cpu.sp == 0x8006u);
    assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u);
    assert(get16(ram, cpu.ss, 0x7FFEu) == 0xBEEFu);
    assert(get16(ram, cpu.ss, 0x7FFCu) == answer);
    free(ram);
}

int main(void)
{
    uint8_t *ram = calloc(PR_U3C2_0000_RAM_SIZE, 1u);
    pr_u3c2_0000_cpu cpu = {0};
    assert(ram != NULL);
    assert(pr_u3c2_sign_entry1_0000(NULL, PR_U3C2_0000_RAM_SIZE,
                                     &cpu) == PR_U3C2_0000_ARGUMENT);
    assert(pr_u3c2_sign_entry1_0000(ram, PR_U3C2_0000_RAM_SIZE - 1u,
                                     &cpu) == PR_U3C2_0000_ARGUMENT);
    cpu.ip = 1u;
    assert(pr_u3c2_sign_entry1_0000(ram, PR_U3C2_0000_RAM_SIZE,
                                     &cpu) == PR_U3C2_0000_ARGUMENT);
    free(ram);
    run(0x8000u, 0xFFFFu, 0x0286u);
    run(0xFFFFu, 0xFFFFu, 0x0286u);
    run(0u, 0u, 0x0246u);
    run(1u, 1u, 0x0202u);
    run(0x7FFFu, 1u, 0x0206u);
    return 0;
}
