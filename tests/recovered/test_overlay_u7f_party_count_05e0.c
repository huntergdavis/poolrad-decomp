#include "poolrad/recovered/overlay_u7f_party_count_05e0.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t address(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) &
           (PR_U7F_05E0_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset,
                  uint16_t value)
{
    size_t a = address(segment, offset);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U7F_05E0_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t a = address(segment, offset);
    return (uint16_t)(ram[a] |
       ((uint16_t)ram[(a + 1u) & (PR_U7F_05E0_RAM_SIZE - 1u)] << 8));
}

int main(void)
{
    uint8_t *ram = calloc(PR_U7F_05E0_RAM_SIZE, 1u);
    pr_u7f_05e0_cpu cpu = {
        0xABCDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0xF000u, 0x05E0u, 0x1000u, 0x1111u,
        0xE000u, 0x7202u
    };
    assert(ram != NULL);
    assert(pr_u7f_party_count_05e0(NULL, PR_U7F_05E0_RAM_SIZE,
                                    &cpu) == PR_U7F_05E0_ARGUMENT);
    assert(pr_u7f_party_count_05e0(ram, PR_U7F_05E0_RAM_SIZE - 1u,
                                    &cpu) == PR_U7F_05E0_ARGUMENT);

    put16(ram, cpu.ss, 0x8000u, 0x0712u);
    put16(ram, cpu.ss, 0x8002u, 0xF000u);
    put16(ram, cpu.ds, 0x5D96u, 0x0100u);
    put16(ram, cpu.ds, 0x5D98u, 0x4000u);
    ram[address(0x4000u, 0x0184u)] = 0x00u;
    put16(ram, 0x4000u, 0x0204u, 0x0100u);
    put16(ram, 0x4000u, 0x0206u, 0x4020u);
    ram[address(0x4020u, 0x0184u)] = 0xB3u;
    put16(ram, 0x4020u, 0x0204u, 0x0100u);
    put16(ram, 0x4020u, 0x0206u, 0x4040u);
    ram[address(0x4040u, 0x0184u)] = 0x17u;

    assert(pr_u7f_party_count_05e0(ram, PR_U7F_05E0_RAM_SIZE,
                                    &cpu) == PR_U7F_05E0_OK);
    assert(cpu.ax == 2u && cpu.dx == 0u);
    assert(cpu.es == 0x4040u && cpu.di == 0x0100u);
    assert(cpu.bp == 0x7777u && cpu.sp == 0x8004u);
    assert(cpu.cs == 0xF000u && cpu.ip == 0x0712u);
    assert(cpu.flags == 0x7246u);
    assert(get16(ram, cpu.ss, 0x7FFEu) == 0x7777u);
    assert(ram[address(cpu.ss, 0x7FF8u)] == 2u);
    assert(ram[address(cpu.ss, 0x7FFDu)] == 2u);
    free(ram);
    return 0;
}
