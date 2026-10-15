#include "poolrad/recovered/overlay_u38_effect_scale_clear_2d88.h"

#include <assert.h>
#include <stdlib.h>

#define RAM_SIZE PR_U38_101D_RAM_SIZE

typedef struct {
    int mode;
    int calls;
} fixture;

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (RAM_SIZE - 1u);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = at(seg, off);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & (RAM_SIZE - 1u)] << 8));
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = at(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static int select_target(void *context, uint8_t *ram, size_t size,
                         pr_u38_101d_cpu *cpu)
{
    fixture *f = context;
    uint16_t output_off, output_seg;
    assert(size == RAM_SIZE);
    assert(cpu->cs == 0x3456u && cpu->ip == 0x2E8Bu);
    assert(get16(ram, cpu->ss, cpu->sp) == 0x104Au);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) == 0xF000u);
    output_off = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u));
    output_seg = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u));
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 8u)) == 0x2300u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 10u)) == 0x4000u);
    ++f->calls;
    if (f->mode == 4) return 0;
    put16(ram, output_seg, output_off, 0x2200u);
    put16(ram, output_seg, (uint16_t)(output_off + 2u), 0x5000u);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 1u);
    cpu->bx = 0xBEEFu;
    cpu->flags = 0x0247u;
    return 1;
}

static void run(int mode)
{
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u38_101d_cpu cpu = {0};
    fixture f = {mode, 0};
    assert(ram != NULL);
    cpu.ax = 0xA5CDu;
    cpu.bx = 0x3456u;
    cpu.bp = 0x7777u;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x2D88u;
    cpu.ds = 0x3000u;
    cpu.ss = 0xE000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.cs, 0x1048u, 0x3456u);
    put16(ram, cpu.ss, cpu.sp, 0x1234u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    put16(ram, cpu.ds, 0x5D92u, 0x2300u);
    put16(ram, cpu.ds, 0x5D94u, 0x4000u);
    if (mode != 0) {
        put16(ram, 0x4000u, 0x23CCu, 0x2200u);
        put16(ram, 0x4000u, 0x23CEu, 0x5000u);
    }
    ram[at(0x5000u, 0x2232u)] = mode == 1 ? 1u : 0u;
    ram[at(0x5000u, 0x2231u)] = mode == 2 ? 0xB1u : 0u;
    ram[at(cpu.ds, 0x6818u)] = 5u;
    if (mode == 4) {
        assert(pr_u38_effect_scale_clear_2d88(ram, RAM_SIZE, &cpu,
                                        select_target, &f) ==
               PR_U38_2D88_CALL_ABORT);
        assert(cpu.cs == 0x3456u && cpu.ip == 0x2E8Bu);
    } else {
        assert(pr_u38_effect_scale_clear_2d88(ram, RAM_SIZE, &cpu,
                                        select_target, &f) == PR_U38_2D88_OK);
        assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u &&
               cpu.sp == 0x800Eu && cpu.bp == 0x7777u);
        assert(ram[at(cpu.ds, 0x6818u)] ==
               (mode == 1 ? 5u : (mode == 2 ? 2u : 0u)));
        assert(cpu.di == (mode == 0 ? 0x2300u : 0x2200u));
        assert(cpu.es == (mode == 0 ? 0x4000u : 0x5000u));
        assert(get16(ram, cpu.ss, 0x7FF2u) == 0x2D9Au);
        assert(get16(ram, cpu.ss, 0x7FF4u) == 0xF000u);
    }
    assert(f.calls == (mode == 0 ? 0 : 1));
    free(ram);
}

int main(void)
{
    for (int mode = 0; mode < 5; ++mode) run(mode);
    return 0;
}
