#include "poolrad/recovered/overlay_u38_link_message_2bc8.h"

#include <assert.h>
#include <stdlib.h>

#define RAM_SIZE PR_U38_101D_RAM_SIZE

typedef struct {
    int calls;
    int abort;
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

static int dispatch(void *context, uint8_t *ram, size_t size,
                    pr_u38_101d_cpu *cpu)
{
    fixture *f = context;
    assert(size == RAM_SIZE);
    assert(cpu->cs == 0xF000u && cpu->ip == 0x0FACu);
    assert(cpu->sp == 0x7FF0u);
    assert(get16(ram, cpu->ss, cpu->sp) == 0x2C0Bu);
    assert(get16(ram, cpu->ss, cpu->sp + 2u) == 0xF000u);
    assert(get16(ram, cpu->ss, cpu->sp + 4u) == 0x7232u);
    assert(get16(ram, cpu->ss, cpu->sp + 6u) == 0xA103u);
    assert(get16(ram, cpu->ss, cpu->sp + 8u) == 0xA104u);
    ++f->calls;
    if (f->abort) return 0;
    put16(ram, cpu->ss, (uint16_t)(cpu->sp - 2u), cpu->bp);
    ram[at(cpu->ds, 0x6818u)] = 0x5Au;
    cpu->ax = 0xBEEFu;
    return 1;
}

static void run(int mode)
{
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u38_101d_cpu cpu = {0};
    fixture f = {0, mode == 5};
    uint8_t marker = mode == 1 || mode == 5 ? 0x57u :
                     (mode == 2 ? 0x58u :
                      (mode == 3 ? 0x56u : 0x80u));
    assert(ram != NULL);
    cpu.ax = 0xA5CDu;
    cpu.bp = 0x7777u;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x2BC8u;
    cpu.ds = 0x3000u;
    cpu.ss = 0xE000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.ss, cpu.sp, 0x1234u);
    put16(ram, cpu.ss, cpu.sp + 2u, 0xD000u);
    for (int i = 0; i < 5; ++i)
        put16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u + 2u * i),
              (uint16_t)(0xA100u + i));
    put16(ram, cpu.ds, 0x5D92u, 0x2300u);
    put16(ram, cpu.ds, 0x5D94u, 0x4000u);
    if (mode != 0) {
        put16(ram, 0x4000u, 0x23CCu, 0x2200u);
        put16(ram, 0x4000u, 0x23CEu, 0x5000u);
    }
    ram[at(0x5000u, 0x222Eu)] = marker;
    ram[at(cpu.ds, 0x6818u)] = 0xA7u;
    if (mode == 5) {
        assert(pr_u38_link_message_2bc8(ram, RAM_SIZE, &cpu,
                                          dispatch, &f) ==
               PR_U38_2BC8_CALL_ABORT);
        assert(cpu.cs == 0xF000u && cpu.ip == 0x0FACu);
    } else {
        assert(pr_u38_link_message_2bc8(ram, RAM_SIZE, &cpu,
                                          dispatch, &f) == PR_U38_2BC8_OK);
        assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u &&
               cpu.sp == 0x800Eu && cpu.bp == 0x7777u);
        assert(cpu.ax == (mode == 1 || mode == 2 ? 0xBEEFu :
                          (mode == 0 ? 0u : 0x7200u)));
        assert(ram[at(cpu.ds, 0x6818u)] ==
               (mode == 1 || mode == 2 ? 0x5Au : 0xA7u));
    }
    assert(f.calls == (mode == 1 || mode == 2 || mode == 5));
    free(ram);
}

int main(void)
{
    for (int mode = 0; mode < 6; ++mode) run(mode);
    return 0;
}
