#include "poolrad/recovered/overlay_u38_roll_clear_2de4.h"

#include <assert.h>
#include <stdlib.h>

#define RAM_SIZE PR_U38_101D_RAM_SIZE

typedef struct {
    uint8_t result;
    int refuse;
    int calls;
} fixture;

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (RAM_SIZE - 1u);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = at(seg, off);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) &
                                      (RAM_SIZE - 1u)] << 8));
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off,
                  uint16_t value)
{
    size_t a = at(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static int roll(void *context, uint8_t *ram, size_t size,
                pr_u38_2de4_cpu *cpu)
{
    fixture *f = context;
    assert(size == RAM_SIZE);
    assert(cpu->cs == 0xB000u && cpu->ip == 0x0048u);
    assert(cpu->sp == 0x7FF6u);
    assert(get16(ram, cpu->ss, cpu->sp) == 0x2DF2u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) == 0xF000u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u)) == 0xA564u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u)) == 0xA501u);
    ++f->calls;
    if (f->refuse) return 0;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | f->result);
    cpu->flags = 0x0247u;
    cpu->ip = get16(ram, cpu->ss, cpu->sp);
    cpu->cs = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return 1;
}

static void run(uint8_t result, uint8_t latch, int refuse)
{
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u38_2de4_cpu cpu = {0};
    fixture f = {result, refuse, 0};
    assert(ram != NULL);
    cpu.ax = 0xA5CDu;
    cpu.bp = 0x7777u;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x2DE4u;
    cpu.ds = 0x3000u;
    cpu.ss = 0xE000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.cs, 0x2DF0u, 0xB000u);
    put16(ram, cpu.ss, cpu.sp, 0x1234u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    ram[at(cpu.ds, 0x6817u)] = latch;
    ram[at(cpu.ds, 0x6818u)] = 0xA7u;
    if (refuse) {
        assert(pr_u38_roll_clear_2de4(ram, RAM_SIZE, &cpu, roll, &f) ==
               PR_U38_2DE4_ROLL_ABORT);
        assert(cpu.cs == 0xB000u && cpu.ip == 0x0048u);
    } else {
        assert(pr_u38_roll_clear_2de4(ram, RAM_SIZE, &cpu, roll, &f) ==
               PR_U38_2DE4_OK);
        assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u);
        assert(cpu.sp == 0x800Eu && cpu.bp == 0x7777u);
        if (result <= 0x1Eu) {
            assert(ram[at(cpu.ds, 0x6817u)] ==
                   (latch == 0x0Bu || latch == 0x35u ? 0u : latch));
            assert(ram[at(cpu.ds, 0x6818u)] ==
                   (latch == 0x0Bu || latch == 0x35u ? 0u : 0xA7u));
            assert(get16(ram, cpu.ss, 0x7FF8u) == 0x2E04u);
            assert(get16(ram, cpu.ss, 0x7FFAu) == 0xF000u);
            assert(get16(ram, cpu.ss, 0x7FFCu) == 0xA535u);
        } else {
            assert(ram[at(cpu.ds, 0x6817u)] == latch);
            assert(ram[at(cpu.ds, 0x6818u)] == 0xA7u);
        }
    }
    assert(f.calls == 1);
    free(ram);
}

int main(void)
{
    for (unsigned result = 0u; result <= 255u; ++result) {
        run((uint8_t)result, 0x0Bu, 0);
        run((uint8_t)result, 0x35u, 0);
        run((uint8_t)result, 0x58u, 0);
    }
    run(0x1Eu, 0x0Bu, 1);
    return 0;
}
