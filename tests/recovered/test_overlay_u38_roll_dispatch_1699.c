#include "poolrad/recovered/overlay_u38_roll_dispatch_1699.h"

#include <assert.h>
#include <stdlib.h>

#define RAM_SIZE PR_U38_101D_RAM_SIZE

typedef struct {
    int calls;
    int abort_on;
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

static int call_stub(void *context, uint8_t *ram, size_t size,
                     pr_u38_101d_cpu *cpu)
{
    fixture *f = context;
    assert(size == RAM_SIZE);
    assert(cpu->sp == 0x7FF2u);
    ++f->calls;
    if (f->calls == 1) {
        assert(cpu->cs == 0x3456u && cpu->ip == 0x0048u);
        assert(get16(ram, cpu->ss, cpu->sp) == 0x16ADu);
        assert(get16(ram, cpu->ss, cpu->sp + 2u) == 0xF000u);
        assert(get16(ram, cpu->ss, cpu->sp + 4u) == 0xA508u);
        assert(get16(ram, cpu->ss, cpu->sp + 6u) == 0xA502u);
    } else {
        assert(cpu->cs == 0xF000u && cpu->ip == 0x15EBu);
        assert(get16(ram, cpu->ss, cpu->sp) == 0x16B7u);
        assert(get16(ram, cpu->ss, cpu->sp + 2u) == 0xF000u);
        assert(get16(ram, cpu->ss, cpu->sp + 4u) == 0u);
        assert(get16(ram, cpu->ss, cpu->sp + 6u) == f->result);
    }
    assert(get16(ram, cpu->ss, cpu->sp + 8u) == 0xA103u);
    assert(get16(ram, cpu->ss, cpu->sp + 10u) == 0xA104u);
    if (f->calls == f->abort_on) return 0;
    put16(ram, cpu->ss, (uint16_t)(cpu->sp - 2u), cpu->bp);
    if (f->calls == 1) {
        ram[at(cpu->ds, 0x680Fu)] = 0x3Cu;
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | f->result);
    } else {
        ram[at(cpu->ds, 0x6818u)] = 0x2Cu;
        cpu->ax = 0xBEEFu;
    }
    return 1;
}

static void run(int mode)
{
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u38_101d_cpu cpu = {0};
    fixture f = {0, mode == 4 ? 1 : (mode == 5 ? 2 : 0),
                 (uint8_t)(mode == 0 ? 0 :
                           (mode == 1 ? 5 :
                            (mode == 2 ? 0x80 : 0xFF)))};
    assert(ram != NULL);
    cpu.ax = 0xA5CDu;
    cpu.bp = 0x7777u;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x1699u;
    cpu.ds = 0x3000u;
    cpu.ss = 0xE000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.cs, 0x16ABu, 0x3456u);
    put16(ram, cpu.ss, cpu.sp, 0x1234u);
    put16(ram, cpu.ss, cpu.sp + 2u, 0xD000u);
    for (int i = 0; i < 5; ++i)
        put16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u + 2u * i),
              (uint16_t)(0xA100u + i));
    if (mode == 4) {
        assert(pr_u38_roll_dispatch_1699(ram, RAM_SIZE, &cpu,
                                           call_stub, &f) ==
               PR_U38_1699_ROLL_ABORT);
        assert(cpu.cs == 0x3456u && cpu.ip == 0x0048u);
    } else if (mode == 5) {
        assert(pr_u38_roll_dispatch_1699(ram, RAM_SIZE, &cpu,
                                           call_stub, &f) ==
               PR_U38_1699_DISPATCH_ABORT);
        assert(cpu.cs == 0xF000u && cpu.ip == 0x15EBu);
    } else {
        assert(pr_u38_roll_dispatch_1699(ram, RAM_SIZE, &cpu,
                                           call_stub, &f) == PR_U38_1699_OK);
        assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u &&
               cpu.sp == 0x800Eu && cpu.bp == 0x7777u);
        assert(cpu.ax == 0xBEEFu);
        assert(ram[at(cpu.ds, 0x680Fu)] == 0x3Cu);
        assert(ram[at(cpu.ds, 0x6818u)] == 0x2Cu);
    }
    assert(f.calls == (mode == 4 ? 1 : 2));
    free(ram);
}

int main(void)
{
    for (int mode = 0; mode < 6; ++mode) run(mode);
    return 0;
}
