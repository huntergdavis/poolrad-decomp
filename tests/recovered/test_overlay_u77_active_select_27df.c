#include "poolrad/recovered/overlay_u77_active_select_27df.h"

#include <assert.h>
#include <stdlib.h>

static size_t address(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) &
           (PR_U77_27DF_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset,
                  uint16_t value)
{
    size_t a = address(segment, offset);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U77_27DF_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t a = address(segment, offset);
    return (uint16_t)(ram[a] |
        ((uint16_t)ram[(a + 1u) & (PR_U77_27DF_RAM_SIZE - 1u)] << 8));
}

int main(void)
{
    uint8_t *ram = calloc(PR_U77_27DF_RAM_SIZE, 1u);
    pr_u77_27df_cpu cpu = {
        0xABCDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0xF000u, 0x27DFu, 0x1000u, 0x1111u,
        0xE000u, 0x7202u
    };
    assert(ram != NULL);
    assert(pr_u77_active_select_27df(NULL, PR_U77_27DF_RAM_SIZE, &cpu) ==
           PR_U77_27DF_ARGUMENT);
    assert(pr_u77_active_select_27df(ram, PR_U77_27DF_RAM_SIZE - 1u,
                                      &cpu) == PR_U77_27DF_ARGUMENT);

    put16(ram, cpu.ss, 0x8000u, 0x0712u);
    put16(ram, cpu.ss, 0x8002u, 0xF000u);
    put16(ram, cpu.ss, 0x8004u, 0x0047u); /* G */
    put16(ram, cpu.ds, 0x5D96u, 0x0100u);
    put16(ram, cpu.ds, 0x5D98u, 0x4000u);
    put16(ram, cpu.ds, 0x5D92u, 0x0100u);
    put16(ram, cpu.ds, 0x5D94u, 0x4020u);
    put16(ram, 0x4000u, 0x0204u, 0x0100u);
    put16(ram, 0x4000u, 0x0206u, 0x4020u);
    put16(ram, 0x4020u, 0x0204u, 0x0100u);
    put16(ram, 0x4020u, 0x0206u, 0x4040u);

    assert(pr_u77_active_select_27df(ram, PR_U77_27DF_RAM_SIZE,
                                      &cpu) == PR_U77_27DF_OK);
    assert(get16(ram, cpu.ds, 0x5D92u) == 0x0100u);
    assert(get16(ram, cpu.ds, 0x5D94u) == 0x4000u);
    assert(cpu.ax == 0x0100u && cpu.dx == 0x4000u);
    assert(cpu.es == 0x4000u && cpu.di == 0x0100u);
    assert(cpu.bp == 0x7777u && cpu.sp == 0x8006u);
    assert(cpu.cs == 0xF000u && cpu.ip == 0x0712u);
    assert(cpu.flags == 0x7246u);
    free(ram);
    return 0;
}
