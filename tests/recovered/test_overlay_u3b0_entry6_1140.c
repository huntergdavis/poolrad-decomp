#include "poolrad/recovered/overlay_u3b0_entry6_1140.h"

#include <assert.h>
#include <stdlib.h>

typedef struct {
    unsigned calls;
    unsigned abort_at;
} fixture;

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U3B0_1140_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t p = physical(seg, off);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U3B0_1140_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t p = physical(seg, off);
    return (uint16_t)(ram[p] |
        ((uint16_t)ram[(p + 1u) & (PR_U3B0_1140_RAM_SIZE - 1u)] << 8));
}

static uint16_t node_off(unsigned i)
{
    return (uint16_t)(0x4000u + i * 0x40u);
}

static uint16_t node_seg(unsigned i)
{
    return (uint16_t)(0x2200u + i * 0x100u);
}

static int alloc(void *context, uint8_t *ram, size_t ram_size,
                 pr_u3b0_1140_cpu *cpu)
{
    fixture *f = context;
    unsigned i = f->calls;
    unsigned j;
    assert(ram_size == PR_U3B0_1140_RAM_SIZE);
    assert(cpu->ip == (i == 0u ? 0x114Au : 0x11A8u));
    assert(get16(ram, cpu->ss, cpu->sp) == 0x002Eu);
    ++f->calls;
    if (f->calls == f->abort_at) return 0;
    for (j = 0u; j < 0x2Eu; ++j)
        ram[physical(node_seg(i), (uint16_t)(node_off(i) + j))] =
            (uint8_t)(0xB0u + i + j);
    cpu->ax = node_off(i);
    cpu->dx = node_seg(i);
    cpu->bx = (uint16_t)(0xB000u + i);
    cpu->flags = 0x0A53u;
    return 1;
}

static void run(uint16_t count, unsigned abort_at, int alias_return)
{
    uint8_t *ram = calloc(PR_U3B0_1140_RAM_SIZE, 1u);
    pr_u3b0_1140_cpu cpu = {0};
    fixture f = {0u, abort_at};
    uint16_t output_seg, output_off;
    unsigned i;
    assert(ram != NULL);
    cpu.ax = 0xA5CDu; cpu.bp = 0x7777u; cpu.sp = 0x8000u;
    cpu.cs = 0xF000u; cpu.ip = 0x1140u;
    cpu.ds = 0x3000u; cpu.ss = 0xE000u;
    output_seg = alias_return ? cpu.ss : cpu.ds;
    output_off = alias_return ? cpu.sp : 0x7000u;
    put16(ram, cpu.ss, cpu.sp, 0x0712u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u), output_off);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 6u), output_seg);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 8u), count);
    if (abort_at != 0u) {
        assert(pr_u3b0_entry6_1140(ram, PR_U3B0_1140_RAM_SIZE,
                                    &cpu, alloc, &f) == PR_U3B0_1140_ALLOC_ABORT);
        assert(f.calls == abort_at);
        assert(cpu.ip == (abort_at == 1u ? 0x114Au : 0x11A8u));
    } else {
        unsigned nodes = count >= 2u && count < 0x8000u ? count : 1u;
        assert(pr_u3b0_entry6_1140(ram, PR_U3B0_1140_RAM_SIZE,
                                    &cpu, alloc, &f) == PR_U3B0_1140_OK);
        assert(f.calls == nodes && cpu.sp == 0x800Au && cpu.bp == 0x7777u);
        assert(cpu.ip == (alias_return ? node_off(0u) : 0x0712u));
        assert(cpu.cs == (alias_return ? node_seg(0u) : 0xD000u));
        if (!alias_return) {
            assert(get16(ram, output_seg, output_off) == node_off(0u));
            assert(get16(ram, output_seg, (uint16_t)(output_off + 2u)) == node_seg(0u));
        }
        for (i = 0u; i < nodes; ++i) {
            assert(ram[physical(node_seg(i), node_off(i))] == 0u);
            assert(ram[physical(node_seg(i), (uint16_t)(node_off(i) + 0x29u))] == 0u);
            assert(get16(ram, node_seg(i), (uint16_t)(node_off(i) + 0x2Au)) ==
                   (i + 1u < nodes ? node_off(i + 1u) : 0u));
            assert(get16(ram, node_seg(i), (uint16_t)(node_off(i) + 0x2Cu)) ==
                   (i + 1u < nodes ? node_seg(i + 1u) : 0u));
            assert(ram[physical(node_seg(i), (uint16_t)(node_off(i) + 1u))] ==
                   (uint8_t)(0xB1u + i));
        }
    }
    free(ram);
}

int main(void)
{
    run(0u, 0u, 0);
    run(1u, 0u, 0);
    run(2u, 0u, 0);
    run(3u, 0u, 0);
    run(0xFFFFu, 0u, 0);
    run(2u, 0u, 1);
    run(2u, 1u, 0);
    run(2u, 2u, 0);
    run(3u, 3u, 0);
    return 0;
}
