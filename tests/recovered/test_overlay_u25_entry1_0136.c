/* Synthetic full-state branch and frame cases for U25 entry 1. */
#include "poolrad/recovered/overlay_u25_entry1_0136.h"

#include <assert.h>
#include <stdlib.h>

#define RAM_SIZE PR_U25_0136_RAM_SIZE

typedef struct {
    unsigned calls;
    uint8_t type;
    uint8_t index;
} fixture;

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (RAM_SIZE - 1u);
}

static uint16_t word(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = at(seg, off);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & (RAM_SIZE - 1u)] << 8));
}

static void put_word(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t a = at(seg, off);
    ram[a] = (uint8_t)v;
    ram[(a + 1u) & (RAM_SIZE - 1u)] = (uint8_t)(v >> 8);
}

static int callee(void *context, pr_u25_0136_call_kind kind,
                  uint8_t *ram, size_t size, pr_u25_0136_cpu *cpu)
{
    fixture *f = context;
    uint16_t sp = cpu->sp;
    uint16_t ah = f->index >= 0x80u ? 0xFF00u : 0u;
    assert(size == RAM_SIZE && f->calls == 0u);
    assert(cpu->cs == 0xF000u && cpu->ss == 0xE000u);
    assert(cpu->bp == 0x7FFEu);
    assert(cpu->di == (f->index >= 0x80u ?
                       (uint16_t)(0xFF00u | f->index) : f->index));
    assert(word(ram, cpu->ss, (uint16_t)(sp + 2u)) == 0xF000u);
    if (kind == PR_U25_0136_CALL_0E0C) {
        assert(f->type == 1u || f->type == 3u || f->type == 0x80u);
        assert(cpu->ip == 0x0E0Cu && sp == 0x7FEEu);
        assert(word(ram, cpu->ss, sp) == 0x0191u);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 4u)) == 0x1234u);
        cpu->sp = (uint16_t)(sp + 6u);
        cpu->ip = 0x0191u;
    } else {
        assert(kind == PR_U25_0136_CALL_06AF);
        assert(f->type == 2u || f->type == 0x81u);
        assert(cpu->ip == 0x06AFu && sp == 0x7FECu);
        assert(word(ram, cpu->ss, sp) == 0x01AAu);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 4u)) == (uint16_t)(ah | 0x12u));
        assert(word(ram, cpu->ss, (uint16_t)(sp + 6u)) == (uint16_t)(ah | 0x34u));
        cpu->sp = (uint16_t)(sp + 8u);
        cpu->ip = 0x01AAu;
    }
    cpu->ax = 0xA55Au;
    cpu->flags = 0xD246u;
    ++f->calls;
    return 1;
}

static void run(uint8_t type, uint8_t index, int without_callee)
{
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u25_0136_cpu cpu = {0};
    fixture f = {0u, type, index};
    pr_u25_0136_io io = {without_callee ? NULL : callee, &f};
    pr_u25_0136_call_kind stopped = PR_U25_0136_CALL_06AF;
    pr_u25_0136_status status;
    uint16_t si = index >= 0x80u ? (uint16_t)(0xFF00u | index) : index;
    uint16_t local = 0xBEEFu;
    assert(ram != NULL);
    cpu.ax = 0xA5CDu; cpu.bp = 0x7777u; cpu.sp = 0x8000u;
    cpu.cs = 0xF000u; cpu.ip = 0x0136u;
    cpu.ds = 0x3000u; cpu.es = 0x4000u; cpu.ss = 0xE000u;
    cpu.cx = 0xCCDEu; cpu.dx = 0xBEE0u; cpu.flags = 0x0202u;
    put_word(ram, cpu.ss, 0x8000u, 0x4567u);
    put_word(ram, cpu.ss, 0x8002u, 0x9876u);
    put_word(ram, cpu.ss, 0x8004u, index);
    put_word(ram, cpu.ss, 0x7FFCu, local);
    ram[at(cpu.ds, (uint16_t)(0x7085u + si))] = 0x34u;
    ram[at(cpu.ds, (uint16_t)(0x7045u + si))] = 0x12u;
    ram[at(cpu.ds, (uint16_t)(0x7005u + si))] = type;
    status = pr_u25_entry1_0136(ram, RAM_SIZE, &cpu, &io, &stopped);
    if (without_callee) {
        assert(status == PR_U25_0136_CALLEE_REQUIRED);
        assert(f.calls == 0u && stopped == PR_U25_0136_CALL_0E0C);
        assert(cpu.cs == 0xF000u && cpu.ip == 0x0E0Cu);
    } else {
        assert(status == PR_U25_0136_RETURNED);
        assert(cpu.bp == 0x7777u && cpu.sp == 0x8006u);
        assert(cpu.cs == 0x9876u && cpu.ip == 0x4567u);
        assert(cpu.di == si && cpu.es == 0x4000u);
        if (type == 0u) {
            assert(f.calls == 0u && cpu.ax == 0x0034u);
            assert(stopped == PR_U25_0136_CALL_06AF);
        } else if (type == 0x55u) {
            assert(f.calls == 0u && cpu.ax == local);
            assert(word(ram, cpu.ss, 0x7FFCu) == local);
        } else {
            assert(f.calls == 1u && cpu.ax == 0xA55Au);
            assert(word(ram, cpu.ss, 0x7FFCu) == 0xA55Au);
            assert(cpu.flags == 0xD246u);
            assert(stopped == ((type == 1u || type == 3u || type == 0x80u) ?
                               PR_U25_0136_CALL_0E0C : PR_U25_0136_CALL_06AF));
        }
    }
    free(ram);
}

int main(void)
{
    run(0u, 2u, 0);
    run(1u, 2u, 0);
    run(3u, 2u, 0);
    run(0x80u, 2u, 0);
    run(2u, 2u, 0);
    run(0x81u, 0xFFu, 0);
    run(0x55u, 2u, 0);
    run(1u, 2u, 1);
    return 0;
}
