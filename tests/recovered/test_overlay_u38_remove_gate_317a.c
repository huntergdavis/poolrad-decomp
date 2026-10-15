#include "poolrad/recovered/overlay_u38_remove_gate_317a.h"

#include <assert.h>
#include <stdlib.h>

#define RAM_SIZE PR_U38_101D_RAM_SIZE

typedef struct {
    int calls;
    int refuse;
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

static int remove_item(void *context, uint8_t *ram, size_t size,
                       pr_u38_317a_cpu *cpu)
{
    fixture *f = context;
    assert(size == RAM_SIZE);
    assert(cpu->cs == 0xB000u && cpu->ip == 0x002Au);
    assert(cpu->sp == 0x7FF0u);
    assert(get16(ram, cpu->ss, cpu->sp) == 0x3197u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) == 0xF000u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u)) == 0u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u)) == 0u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 8u)) == 0xA517u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 10u)) == 0xA103u);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 12u)) == 0xA104u);
    ++f->calls;
    if (f->refuse) return 0;
    cpu->ax = 0xBEEFu;
    cpu->ip = get16(ram, cpu->ss, cpu->sp);
    cpu->cs = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 14u);
    return 1;
}

static void run(uint8_t gate, int refuse)
{
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u38_317a_cpu cpu = {0};
    fixture f = {0, refuse};
    assert(ram != NULL);
    cpu.ax = 0xA5CDu;
    cpu.bp = 0x7777u;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x317Au;
    cpu.ss = 0xE000u;
    cpu.ds = 0x3000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.cs, 0x3195u, 0xB000u);
    put16(ram, cpu.ss, cpu.sp, 0x1234u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    for (unsigned i = 0u; i < 5u; ++i)
        put16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u + 2u * i),
              (uint16_t)(0xA100u + i));
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u),
          (uint16_t)(0xA100u | gate));
    if (refuse) {
        assert(pr_u38_remove_gate_317a(ram, RAM_SIZE, &cpu,
                                       remove_item, &f) ==
               PR_U38_317A_CALL_ABORT);
        assert(cpu.cs == 0xB000u && cpu.ip == 0x002Au);
    } else {
        assert(pr_u38_remove_gate_317a(ram, RAM_SIZE, &cpu,
                                       remove_item, &f) == PR_U38_317A_OK);
        assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u);
        assert(cpu.sp == 0x800Eu && cpu.bp == 0x7777u);
        assert(cpu.ax == (gate == 0u ? 0xA5CDu : 0xBEEFu));
        assert((cpu.flags & 0x40u) != 0u);
        if (gate != 0u) {
            assert(get16(ram, cpu.ss, 0x7FF0u) == 0x3197u);
            assert(get16(ram, cpu.ss, 0x7FF2u) == 0xF000u);
        }
    }
    assert(f.calls == (gate != 0u ? 1 : 0));
    free(ram);
}

int main(void)
{
    run(0u, 0);
    run(1u, 0);
    run(0x80u, 0);
    run(0xFFu, 0);
    run(1u, 1);
    return 0;
}
