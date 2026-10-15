#include "poolrad/recovered/overlay_u38_character_compare_1276.h"

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
    const uint8_t first[] = {3u, 0x31u, 0x32u, 0x33u};
    const uint8_t second[] = {2u, 0x44u, 0x45u};
    const uint8_t *selected = mode == 1 ? first : second;
    size_t selected_size = mode == 1 ? sizeof(first) : sizeof(second);
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
    cpu.ip = 0x1276u;
    cpu.ds = 0x3000u;
    cpu.es = 0x1111u;
    cpu.ss = 0xE000u;
    cpu.flags = 0x76D3u;
    put16(ram, cpu.ss, cpu.sp, 0x1234u);
    put16(ram, cpu.ss, cpu.sp + 2u, 0xD000u);
    for (int i = 0; i < 5; ++i)
        put16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u + 2u * i),
              (uint16_t)(0xA100u + i));
    put16(ram, cpu.ds, 0x5D92u, 0x2200u);
    put16(ram, cpu.ds, 0x5D94u, 0x4000u);
    put16(ram, cpu.cs, 0x1293u, 0x7100u);
    put16(ram, cpu.cs, 0x12A5u, 0x7200u);
    for (size_t i = 0; i < sizeof(first); ++i)
        ram[at(cpu.cs, (uint16_t)(0x1268u + i))] = first[i];
    for (size_t i = 0; i < sizeof(second); ++i)
        ram[at(cpu.cs, (uint16_t)(0x1270u + i))] = second[i];
    for (size_t i = 0; i < selected_size; ++i)
        ram[at(0x4000u, (uint16_t)(0x2200u + i))] = selected[i];
    if (mode == 3)
        ram[at(0x4000u, 0x2201u)] = 0x7Fu;
    ram[at(0x4000u, 0x229Fu)] = mode == 0 ? 2u : 1u;
    ram[at(cpu.ds, 0x6822u)] = 0xA6u;
    assert(pr_u38_character_compare_1276(ram, RAM_SIZE, &cpu) ==
           PR_U38_1276_OK);
    assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u &&
           cpu.sp == 0x800Eu && cpu.bp == 0x7777u);
    assert(ram[at(0x3000u, 0x6822u)] ==
           (mode == 1 || mode == 2 ? 0xA2u : 0xA6u));
    if (mode != 0) {
        assert(cpu.dx == 0x3000u);
        assert((cpu.flags & 0x400u) == 0u);
    }
    free(ram);
}

int main(void)
{
    for (int mode = 0; mode < 4; ++mode) run(mode);
    return 0;
}
