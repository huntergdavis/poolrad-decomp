#include "poolrad/recovered/overlay_u38_character_compare_loop_11f9.h"

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
    cpu.cx = 0x5678u;
    cpu.bp = 0x7777u;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x11F9u;
    cpu.ds = 0x3000u;
    cpu.ss = 0xE000u;
    cpu.flags = 0x76D3u;
    put16(ram, cpu.ss, 0x8000u, 0x1234u);
    put16(ram, cpu.ss, 0x8002u, 0xD000u);
    put16(ram, cpu.ss, 0x800Au, 0x2100u);
    put16(ram, cpu.ss, 0x800Cu, 0x5000u);
    put16(ram, 0x5000u, 0x2208u, 0x2300u);
    put16(ram, 0x5000u, 0x220Au, 0x5100u);
    put16(ram, 0x5100u, 0x230Au, 0x2700u);
    put16(ram, 0x5100u, 0x230Cu, 0x5200u);
    put16(ram, cpu.ds, 0x5D92u, 0x2200u);
    put16(ram, cpu.ds, 0x5D94u, 0x4000u);
    ram[at(0x4000u, 0x229Fu)] = mode == 0 ? 2u : 1u;
    ram[at(0x4000u, 0x226Cu)] = mode == 1 ? 3u : 2u;
    ram[at(cpu.ds, 0x6822u)] = 0xA6u;
    put16(ram, cpu.cs, 0x1253u, 0x7100u);
    ram[at(0x5200u, 0x2700u)] = 1u;
    ram[at(0x5200u, 0x2701u)] = 0x41u;
    for (unsigned i = 1u; i <= 8u; ++i) {
        uint16_t off = (uint16_t)(0x3F2u + (i << 4));
        ram[at(cpu.ds, off)] = 1u;
        ram[at(cpu.ds, (uint16_t)(off + 1u))] =
            mode == 3 ? 0x41u : 0x42u;
    }
    assert(pr_u38_character_compare_loop_11f9(ram, RAM_SIZE, &cpu) ==
           PR_U38_11F9_OK);
    assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u &&
           cpu.sp == 0x800Eu && cpu.bp == 0x7777u);
    assert(ram[at(cpu.ds, 0x6822u)] == (mode == 3 ? 0x86u : 0xA6u));
    assert(ram[at(cpu.ds, 0x6826u)] == 0x00u &&
           ram[at(cpu.ds, 0x6827u)] == 0x27u);
    assert(ram[at(cpu.ds, 0x6828u)] == 0x00u &&
           ram[at(cpu.ds, 0x6829u)] == 0x52u);
    free(ram);
}

int main(void)
{
    for (int mode = 0; mode < 4; ++mode) run(mode);
    return 0;
}
