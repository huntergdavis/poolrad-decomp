#include "poolrad/recovered/overlay_u38_effect_match_12b2.h"

#include <assert.h>
#include <stdlib.h>

#define RAM_SIZE PR_U38_101D_RAM_SIZE

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = at(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static void run(int mode)
{
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u38_101d_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ax = 0xA5CDu;
    cpu.bx = 0x3456u;
    cpu.cx = 0x5678u;
    cpu.dx = 0x789Au;
    cpu.si = 0x9ABCu;
    cpu.di = 0xBCDEu;
    cpu.bp = 0x7777u;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x12B2u;
    cpu.ds = 0x3000u;
    cpu.es = 0x1111u;
    cpu.ss = 0xE000u;
    cpu.flags = 0x76D3u;
    put16(ram, cpu.ss, cpu.sp, 0x1234u);
    put16(ram, cpu.ss, 0x8002u, 0xD000u);
    put16(ram, cpu.ss, 0x8004u, 0xA145u);
    put16(ram, cpu.ss, 0x8006u, 0x2200u);
    put16(ram, cpu.ss, 0x8008u, 0x4000u);
    put16(ram, cpu.ss, 0x800Au, 0x2100u);
    put16(ram, cpu.ss, 0x800Cu, 0x5000u);
    ram[at(0x4000u, 0x2203u)] = (mode & 1) ? 0x10u : 0u;
    ram[at(0x5000u, 0x220Eu)] = (mode & 2) ? 1u : 0u;
    ram[at(cpu.ds, 0x6816u)] = 0x7Fu;
    ram[at(cpu.ds, 0x6822u)] = 0xFFu;
    assert(pr_u38_effect_match_12b2(ram, RAM_SIZE, &cpu) == PR_U38_12B2_OK);
    assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u &&
           cpu.sp == 0x800Eu && cpu.bp == 0x7777u);
    if ((mode & 1) == ((mode & 2) >> 1)) {
        assert(ram[at(cpu.ds, 0x6816u)] == 0x80u);
        assert(ram[at(cpu.ds, 0x6822u)] == 0u);
    } else {
        assert(ram[at(cpu.ds, 0x6816u)] == 0x7Eu);
        assert(ram[at(cpu.ds, 0x6822u)] == 0xFEu);
    }
    free(ram);
}

int main(void)
{
    for (int mode = 0; mode < 4; ++mode) run(mode);
    return 0;
}
