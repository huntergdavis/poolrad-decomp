#include "poolrad/recovered/overlay_u38_target_reset_call_133f.h"

#include <assert.h>
#include <stdlib.h>

#define RAM_SIZE PR_U38_101D_RAM_SIZE

typedef struct { uint8_t result; int calls; int abort; } fixture;

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

static int reset(void *context, uint8_t *ram, size_t size,
                 pr_u38_101d_cpu *cpu)
{
    fixture *f = context;
    assert(size == RAM_SIZE);
    assert(cpu->cs == 0xB123u && cpu->ip == 0x2637u);
    assert(cpu->sp == 0x7FF6u);
    assert(get16(ram, cpu->ss, cpu->sp) == 0x134Du);
    assert(get16(ram, cpu->ss, cpu->sp + 2u) == 0xF000u);
    assert(get16(ram, cpu->ss, cpu->sp + 4u) == 0xA103u);
    assert(get16(ram, cpu->ss, cpu->sp + 6u) == 0xA104u);
    ++f->calls;
    if (f->abort) return 0;
    ram[at(cpu->ds, 0x6816u)] = 0x5Au;
    cpu->ax = (uint16_t)(0xA500u | f->result);
    return 1;
}

static void run(uint8_t result, int abort)
{
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u38_101d_cpu cpu = {0};
    fixture f = {result, 0, abort};
    assert(ram != NULL);
    cpu.ax = 0x2211u;
    cpu.bp = 0x7777u;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x133Fu;
    cpu.ds = 0x3000u;
    cpu.ss = 0xE000u;
    cpu.flags = 0x0AD3u;
    put16(ram, cpu.cs, 0x134Bu, 0xB123u);
    put16(ram, cpu.ss, cpu.sp, 0x1234u);
    put16(ram, cpu.ss, cpu.sp + 2u, 0xD000u);
    for (int i = 0; i < 5; ++i)
        put16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u + 2u * i),
              (uint16_t)(0xA100u + i));
    if (abort) {
        assert(pr_u38_target_reset_call_133f(ram, RAM_SIZE, &cpu,
                                               reset, &f) ==
               PR_U38_133F_CALL_ABORT);
        assert(cpu.cs == 0xB123u && cpu.ip == 0x2637u &&
               cpu.sp == 0x7FF6u);
    } else {
        assert(pr_u38_target_reset_call_133f(ram, RAM_SIZE, &cpu,
                                               reset, &f) == PR_U38_133F_OK);
        assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u &&
               cpu.sp == 0x800Eu && cpu.bp == 0x7777u);
        assert(cpu.ax == (uint16_t)(0xA500u | result));
        assert((cpu.flags & 0x40u) == (result == 0u ? 0x40u : 0u));
        assert(ram[at(cpu.ds, 0x6816u)] == 0x5Au);
    }
    assert(f.calls == 1);
    free(ram);
}

int main(void)
{
    run(0u, 0);
    run(1u, 0);
    run(0x80u, 0);
    run(1u, 1);
    return 0;
}
