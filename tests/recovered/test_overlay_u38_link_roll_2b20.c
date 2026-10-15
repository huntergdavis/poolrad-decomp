#include "poolrad/recovered/overlay_u38_link_roll_2b20.h"

#include <assert.h>
#include <stdlib.h>

#define RAM_SIZE PR_U9E_0E2E_RAM_SIZE

typedef struct {
    int mode;
    int far_calls;
    int near_calls;
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

static int roll(void *context, uint8_t *ram, size_t size,
                pr_u9e_0e2e_cpu *cpu)
{
    fixture *f = context;
    uint8_t value = f->mode == 3 ? 0x0Fu : (f->mode == 4 ? 0xFFu : 0u);
    assert(size == RAM_SIZE);
    assert(cpu->cs == 0xB000u && cpu->ip == 0x0DE3u);
    assert(get16(ram, cpu->ss, cpu->sp) == 0x0E46u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) == 0xB000u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u)) == 0x7206u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u)) == 0x7201u);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
    cpu->bx = 0xBEEFu;
    cpu->flags = 0x0247u;
    ++f->near_calls;
    return 1;
}

static int roll_latch(void *context, uint8_t *ram, size_t size,
                      pr_u38_2b20_cpu *cpu)
{
    fixture *f = context;
    assert(size == RAM_SIZE);
    assert(cpu->cs == 0xB000u && cpu->ip == 0x004Du);
    assert(get16(ram, cpu->ss, cpu->sp) == 0x2B57u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) == 0xF000u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u)) == 0x7206u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u)) == 0x7201u);
    ++f->far_calls;
    if (f->mode == 5) return 0;
    cpu->ip = 0x0E2Eu; /* loader stub dispatch */
    assert(pr_u9e_entry9_roll_latch_0e2e(ram, size, cpu, roll, f) ==
           PR_U9E_0E2E_OK);
    return 1;
}

static void run(int mode)
{
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u38_2b20_cpu cpu = {0};
    fixture f = {mode, 0, 0};
    assert(ram != NULL);
    cpu.ax = 0xA5CDu;
    cpu.bp = 0x7777u;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x2B20u;
    cpu.ds = 0x3000u;
    cpu.ss = 0xE000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.cs, 0x2B55u, 0xB000u);
    put16(ram, cpu.ss, cpu.sp, 0x1234u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    put16(ram, cpu.ds, 0x5D92u, 0x2300u);
    put16(ram, cpu.ds, 0x5D94u, 0x4000u);
    if (mode != 0) {
        put16(ram, 0x4000u, 0x23CCu, 0x2200u);
        put16(ram, 0x4000u, 0x23CEu, 0x5000u);
    }
    ram[at(0x5000u, 0x222Eu)] = mode >= 2 ? 0x55u : 0x54u;
    ram[at(cpu.ds, 0x6818u)] = 0xA7u;
    ram[at(cpu.ds, 0x681Cu)] = 0xB8u;
    if (mode == 5) {
        assert(pr_u38_link_roll_2b20(ram, RAM_SIZE, &cpu,
                                      roll_latch, &f) ==
               PR_U38_2B20_CALL_ABORT);
        assert(cpu.cs == 0xB000u && cpu.ip == 0x004Du);
    } else {
        assert(pr_u38_link_roll_2b20(ram, RAM_SIZE, &cpu,
                                      roll_latch, &f) == PR_U38_2B20_OK);
        assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u &&
               cpu.sp == 0x800Eu && cpu.bp == 0x7777u);
        assert(ram[at(cpu.ds, 0x6818u)] ==
               (mode < 2 ? 0xA7u : (mode == 3 ? 0x10u :
                                     (mode == 4 ? 0u : 1u))));
        assert(ram[at(cpu.ds, 0x681Cu)] == (mode < 2 ? 0xB8u : 1u));
        if (mode == 3) assert((cpu.flags & 0x10u) != 0u);
        if (mode == 4) assert(cpu.ax == 0x0100u);
        if (mode >= 2) {
            assert(get16(ram, cpu.ss, 0x7FF2u) == 0x2B57u);
            assert(get16(ram, cpu.ss, 0x7FF4u) == 0xF000u);
        }
    }
    assert(f.far_calls == (mode >= 2 ? 1 : 0));
    assert(f.near_calls == (mode >= 2 && mode != 5 ? 1 : 0));
    free(ram);
}

int main(void)
{
    for (int mode = 0; mode < 6; ++mode) run(mode);
    return 0;
}
