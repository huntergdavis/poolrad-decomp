#include "poolrad/recovered/overlay_u38_roll_effect_27d9.h"

#include <assert.h>
#include <stdlib.h>

#define RAM_SIZE PR_U38_101D_RAM_SIZE

typedef struct {
    int mode;
    int rolls;
    int effects;
    uint8_t result;
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

static int roll_stub(void *context, uint8_t *ram, size_t size,
                     pr_u38_101d_cpu *cpu)
{
    fixture *f = context;
    assert(size == RAM_SIZE);
    assert(cpu->cs == 0x3456u && cpu->ip == 0x0048u);
    assert(get16(ram, cpu->ss, cpu->sp) == 0x2802u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) == 0xF000u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u)) == 0xA506u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u)) == 0xA503u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 8u)) == 0xA566u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 10u)) == 0xA103u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 12u)) == 0xA104u);
    ++f->rolls;
    if (f->mode == 5) return 0;
    put16(ram, cpu->ss, (uint16_t)(cpu->sp - 2u), cpu->bp);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | f->result);
    return 1;
}

static int effect_stub(void *context, uint8_t *ram, size_t size,
                       pr_u38_101d_cpu *cpu)
{
    fixture *f = context;
    assert(size == RAM_SIZE);
    assert(cpu->cs == 0x4567u && cpu->ip == 0x0052u);
    assert(get16(ram, cpu->ss, cpu->sp) == 0x2810u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) == 0xF000u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u)) == 1u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u)) == 0x00FFu);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 8u)) == f->result);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 10u)) == 0xA566u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 12u)) == 0xA103u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 14u)) == 0xA104u);
    ++f->effects;
    if (f->mode == 6) return 0;
    put16(ram, cpu->ss, (uint16_t)(cpu->sp - 2u), cpu->bp);
    ram[at(cpu->ds, 0x6818u)] = 0x5Au;
    cpu->ax = 0xBEEFu;
    return 1;
}

static void run(int mode)
{
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u38_101d_cpu cpu = {0};
    fixture f = {mode, 0, 0, (uint8_t)(mode == 4 ? 0xFFu : 7u)};
    pr_u38_27d9_status result;
    assert(ram != NULL);
    cpu.ax = 0xA5CDu;
    cpu.bp = 0x7777u;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x27D9u;
    cpu.ds = 0x3000u;
    cpu.ss = 0xE000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.cs, 0x2800u, 0x3456u);
    put16(ram, cpu.cs, 0x280Eu, 0x4567u);
    put16(ram, cpu.ss, cpu.sp, 0x1234u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    for (int i = 0; i < 5; ++i)
        put16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u + 2u * (unsigned)i),
              (uint16_t)(0xA100u + (unsigned)i));
    ram[at(cpu.ds, 0x6819u)] =
        mode == 1 ? 1u : (mode == 2 ? 0x10u : (mode == 3 ? 0x11u : 0u));
    ram[at(cpu.ds, 0x6818u)] = 0xA7u;
    result = pr_u38_roll_effect_27d9(ram, RAM_SIZE, &cpu,
                                      roll_stub, effect_stub, &f);
    if (mode == 5) {
        assert(result == PR_U38_27D9_ROLL_ABORT);
        assert(cpu.cs == 0x3456u && cpu.ip == 0x0048u);
    } else if (mode == 6) {
        assert(result == PR_U38_27D9_EFFECT_ABORT);
        assert(cpu.cs == 0x4567u && cpu.ip == 0x0052u);
    } else {
        assert(result == PR_U38_27D9_OK);
        assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u &&
               cpu.sp == 0x800Eu && cpu.bp == 0x7777u);
        assert(ram[at(cpu.ds, 0x6818u)] ==
               (mode == 0 || mode == 4 ? 0x5Au : 0xA7u));
        if (mode == 0 || mode == 4) assert(cpu.ax == 0xBEEFu);
    }
    assert(f.rolls == (mode == 1 || mode == 2 || mode == 3 ? 0 : 1));
    assert(f.effects == (mode == 1 || mode == 2 || mode == 3 || mode == 5 ? 0 : 1));
    free(ram);
}

int main(void)
{
    for (int mode = 0; mode < 7; ++mode) run(mode);
    return 0;
}
