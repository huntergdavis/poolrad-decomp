#include "poolrad/recovered/overlay_u6a_sort_0824.h"

#include <assert.h>
#include <stdlib.h>

static size_t address(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) &
           (PR_U6A_0824_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset,
                  uint16_t value)
{
    size_t a = address(segment, offset);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U6A_0824_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

int main(void)
{
    uint8_t *ram = calloc(PR_U6A_0824_RAM_SIZE, 1u);
    pr_u6a_0824_cpu cpu = {
        0xABCDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0xF000u, 0x0824u, 0x1000u, 0x1111u,
        0xE000u, 0x7202u
    };
    assert(ram != NULL);
    assert(pr_u6a_sort_0824(NULL, PR_U6A_0824_RAM_SIZE, &cpu) ==
           PR_U6A_0824_ARGUMENT);
    assert(pr_u6a_sort_0824(ram, PR_U6A_0824_RAM_SIZE - 1u, &cpu) ==
           PR_U6A_0824_ARGUMENT);

    put16(ram, cpu.ss, cpu.sp, 0x0712u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xF000u);
    put16(ram, cpu.ds, 0x5D92u, 0x0100u);
    put16(ram, cpu.ds, 0x5D94u, 0x4000u);
    for (unsigned i = 0; i < 21u; ++i)
        ram[address(0x4000u, (uint16_t)(0x0117u + i))] =
            (uint8_t)(0x80u | (20u - i));

    assert(pr_u6a_sort_0824(ram, PR_U6A_0824_RAM_SIZE, &cpu) ==
           PR_U6A_0824_OK);
    for (unsigned i = 0; i < 21u; ++i)
        assert(ram[address(0x4000u, (uint16_t)(0x0117u + i))] ==
               (uint8_t)(0x80u | i));
    assert(cpu.bp == 0x7777u && cpu.sp == 0x8004u);
    assert(cpu.cs == 0xF000u && cpu.ip == 0x0712u);
    assert(cpu.flags == 0x7246u);
    free(ram);
    return 0;
}
