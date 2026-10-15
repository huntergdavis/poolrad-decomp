#include "poolrad/recovered/overlay_u38_target_dispatch_1086.h"

#include <assert.h>
#include <stdlib.h>

#define RAM_SIZE PR_U38_101D_RAM_SIZE

typedef struct {
    int mode;
    int selects;
    int dispatches;
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
    uint16_t out_off, out_seg;
    assert(size == RAM_SIZE);
    assert(cpu->cs == 0x3456u && cpu->ip == 0x2E8Bu);
    assert(get16(ram, cpu->ss, cpu->sp) == 0x104Au);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) == 0xF000u);
    out_off = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u));
    out_seg = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u));
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 8u)) == 0x2300u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 10u)) == 0x4000u);
    ++f->selects;
    if ((f->mode == 3 && f->selects == 1) ||
        (f->mode == 4 && f->selects == 2)) return 0;
    put16(ram, out_seg, out_off, 0x2200u);
    put16(ram, out_seg, (uint16_t)(out_off + 2u), 0x5000u);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 1u);
    cpu->bx = 0xBEEFu;
    cpu->flags = 0x0247u;
    return 1;
}

static int dispatch(void *context, uint8_t *ram, size_t size,
                    pr_u38_101d_cpu *cpu)
{
    fixture *f = context;
    assert(size == RAM_SIZE);
    assert(cpu->cs == 0xF000u && cpu->ip == 0x0FACu);
    assert(get16(ram, cpu->ss, cpu->sp) == 0x10BDu);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) == 0xF000u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u)) == 0x2264u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u)) == 0xA103u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 8u)) == 0xA104u);
    ++f->dispatches;
    if (f->mode == 5) return 0;
    put16(ram, cpu->ss, (uint16_t)(cpu->sp - 2u), cpu->bp);
    ram[at(cpu->ds, 0x6818u)] = 0x5Au;
    cpu->ax = 0xBEEFu;
    return 1;
}

static void run(int mode)
{
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u38_101d_cpu cpu = {0};
    fixture f = {mode, 0, 0};
    pr_u38_1086_status result;
    assert(ram != NULL);
    cpu.ax = 0xA5CDu;
    cpu.bx = 0x3456u;
    cpu.bp = 0x7777u;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x1086u;
    cpu.ds = 0x3000u;
    cpu.ss = 0xE000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.cs, 0x1048u, 0x3456u);
    put16(ram, cpu.ss, cpu.sp, 0x1234u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    for (int i = 0; i < 5; ++i)
        put16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u + 2u * (unsigned)i),
              (uint16_t)(0xA100u + (unsigned)i));
    put16(ram, cpu.ds, 0x5D92u, 0x2300u);
    put16(ram, cpu.ds, 0x5D94u, 0x4000u);
    if (mode != 0) {
        put16(ram, 0x4000u, 0x23CCu, 0x2200u);
        put16(ram, 0x4000u, 0x23CEu, 0x5000u);
    }
    ram[at(cpu.ds, 0x5600u)] = 0u;
    ram[at(0x5000u, 0x2232u)] = mode == 1 ? 1u : 0u;
    ram[at(cpu.ds, 0x6818u)] = 0xA7u;
    result = pr_u38_target_dispatch_1086(ram, RAM_SIZE, &cpu,
                                          select_target, dispatch, &f);
    if (mode == 3 || mode == 4) {
        assert(result == PR_U38_1086_SELECT_ABORT);
        assert(cpu.cs == 0x3456u && cpu.ip == 0x2E8Bu);
    } else if (mode == 5) {
        assert(result == PR_U38_1086_DISPATCH_ABORT);
        assert(cpu.cs == 0xF000u && cpu.ip == 0x0FACu);
    } else {
        assert(result == PR_U38_1086_OK);
        assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u &&
               cpu.sp == 0x800Eu && cpu.bp == 0x7777u);
        assert(ram[at(cpu.ds, 0x6818u)] == (mode == 2 ? 0x5Au : 0xA7u));
    }
    assert(f.selects == (mode == 0 ? 0 : (mode == 3 ? 1 : 2)));
    assert(f.dispatches == (mode == 2 || mode == 5));
    free(ram);
}

int main(void)
{
    for (int mode = 0; mode < 6; ++mode) run(mode);
    return 0;
}
