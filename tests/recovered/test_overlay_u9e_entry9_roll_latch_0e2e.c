#include "poolrad/recovered/overlay_u9e_entry9_roll_latch_0e2e.h"
#include "poolrad/recovered/overlay_u9e_entry8_roll.h"

#include <assert.h>
#include <stdlib.h>

#define RAM_SIZE PR_U9E_0E2E_RAM_SIZE

typedef struct {
    uint8_t count, range;
    int calls, abort;
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

static uint32_t reference_seed(uint32_t seed, uint8_t count)
{
    unsigned i;
    for (i = 0; i < count; ++i)
        seed = seed * UINT32_C(0x08088405) + 1u;
    return seed;
}

static int roll(void *context, uint8_t *ram, size_t size,
                pr_u9e_0e2e_cpu *cpu)
{
    fixture *f = context;
    uint16_t range, count;
    uint32_t seed;
    uint8_t al = 0;
    assert(size == RAM_SIZE);
    assert(cpu->cs == 0xF000u && cpu->ip == 0x0DE3u);
    assert(get16(ram, cpu->ss, cpu->sp) == 0x0E46u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) == 0xF000u);
    range = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u));
    count = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u));
    assert(range == (uint16_t)(0xA500u | f->range));
    assert(count == (uint16_t)(0xA500u | f->count));
    ++f->calls;
    if (f->abort) return 0;
    seed = (uint32_t)get16(ram, cpu->ds, 0x4428u) |
           ((uint32_t)get16(ram, cpu->ds, 0x442Au) << 16);
    assert(pr_u9e_entry8_roll(count, range, &seed, &al, NULL) ==
           PR_U9E_ENTRY8_OK);
    put16(ram, cpu->ds, 0x4428u, (uint16_t)seed);
    put16(ram, cpu->ds, 0x442Au, (uint16_t)(seed >> 16));
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | al);
    cpu->bx = 0xBEEFu;
    cpu->flags = 0x0247u;
    return 1;
}

static void run(uint8_t count, uint8_t range, int abort)
{
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u9e_0e2e_cpu cpu = {0};
    fixture f = {count, range, 0, abort};
    uint32_t seed = UINT32_C(0x12345678);
    uint32_t expected = reference_seed(seed, count);
    assert(ram != NULL);
    cpu.ax = 0xA5CDu;
    cpu.bx = 0x3456u;
    cpu.bp = 0x7777u;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x0E2Eu;
    cpu.ds = 0x3000u;
    cpu.ss = 0xE000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.ss, cpu.sp, 0x1234u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u), (uint16_t)(0x7700u | range));
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 6u), (uint16_t)(0x9900u | count));
    put16(ram, cpu.ds, 0x4428u, (uint16_t)seed);
    put16(ram, cpu.ds, 0x442Au, (uint16_t)(seed >> 16));
    if (abort) {
        assert(pr_u9e_entry9_roll_latch_0e2e(ram, RAM_SIZE, &cpu, roll, &f)
               == PR_U9E_0E2E_CALL_ABORT);
        assert(cpu.cs == 0xF000u && cpu.ip == 0x0DE3u && cpu.sp == 0x7FF4u);
    } else {
        assert(pr_u9e_entry9_roll_latch_0e2e(ram, RAM_SIZE, &cpu, roll, &f)
               == PR_U9E_0E2E_OK);
        assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u && cpu.sp == 0x8008u);
        assert(cpu.bp == 0x7777u && cpu.bx == 0xBEEFu && cpu.flags == 0x0247u);
        assert(ram[at(cpu.ds, 0x681Cu)] == count);
        assert(((uint32_t)get16(ram, cpu.ds, 0x4428u) |
                ((uint32_t)get16(ram, cpu.ds, 0x442Au) << 16)) == expected);
        assert(ram[at(cpu.ss, 0x7FFDu)] == (uint8_t)cpu.ax);
    }
    assert(f.calls == 1);
    free(ram);
}

int main(void)
{
    static const uint8_t counts[] = {0, 1, 3, 255};
    static const uint8_t ranges[] = {0, 1, 6, 255};
    size_t i, j;
    for (i = 0; i < sizeof counts; ++i)
        for (j = 0; j < sizeof ranges; ++j)
            run(counts[i], ranges[j], 0);
    run(2u, 6u, 1);
    return 0;
}
