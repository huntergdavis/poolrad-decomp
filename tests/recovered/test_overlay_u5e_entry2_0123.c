/* Synthetic frames for GAME.OVR U5E entry 2, +0123. */
#include "poolrad/recovered/overlay_u5e_entry2_0123.h"

#include <assert.h>
#include <stdlib.h>

#define RAM_SIZE PR_U5E_0123_RAM_SIZE

typedef struct {
    unsigned calls;
    unsigned mode;
    uint8_t doubled;
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

static int callee(void *context, uint8_t *ram, size_t size,
                  pr_u5e_0123_cpu *cpu)
{
    fixture *f = context;
    uint16_t sp = cpu->sp;
    assert(size == RAM_SIZE && f->calls == 0u);
    assert(cpu->cs == 0xD100u && cpu->ip == 0x002Fu);
    assert(cpu->bp == 0x7FFEu && cpu->ss == 0xE000u);
    assert(sp == 0x7FF2u && word(ram, cpu->ss, sp) == 0x017Du);
    assert(word(ram, cpu->ss, (uint16_t)(sp + 2u)) == 0xF000u);
    assert(word(ram, cpu->ss, (uint16_t)(sp + 4u)) == 0x2200u);
    assert(word(ram, cpu->ss, (uint16_t)(sp + 6u)) == 0x4000u);
    assert(word(ram, cpu->ss, (uint16_t)(sp + 8u)) == 0x0012u);
    assert(ram[at(cpu->ds, 0x681Du)] == 1u);
    assert(ram[at(cpu->ds, 0x681Au)] == f->doubled);
    assert(cpu->ax == 0x0012u);
    assert(cpu->di == (f->mode == 1u ? 0x1000u : 0x2200u));
    assert(cpu->es == (f->mode == 1u ? 0x5000u : 0x4000u));
    ++f->calls;
    if (f->mode == 3u) ram[at(cpu->ds, 0x681Au)] = 0x77u;
    cpu->ax = 0xA5AAu;
    cpu->flags = 0xD246u;
    cpu->sp = (uint16_t)(sp + 10u); /* controlled far return with 6 bytes */
    cpu->cs = 0xF000u;
    cpu->ip = 0x017Du;
    return 1;
}

static void run(unsigned mode, int without_callee)
{
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u5e_0123_cpu cpu = {0};
    fixture f = {0u, mode, mode == 1u ? 0x2Au :
                             mode == 2u ? 0xC0u : 2u};
    pr_u5e_0123_io io = {0xD100u, without_callee ? NULL : callee, &f};
    pr_u5e_0123_status status;
    assert(ram != NULL);
    cpu.ax = 0xB5CCu; cpu.bp = 0x7777u; cpu.sp = 0x8000u;
    cpu.cs = 0xF000u; cpu.ip = 0x0123u;
    cpu.ds = 0x3000u; cpu.es = 0x2222u; cpu.ss = 0xE000u;
    cpu.flags = 0x0202u;
    put_word(ram, cpu.ss, 0x8000u, 0x4567u);
    put_word(ram, cpu.ss, 0x8002u, 0x9876u);
    put_word(ram, cpu.ss, 0x8004u, 0x2200u);
    put_word(ram, cpu.ss, 0x8006u, 0x4000u);
    ram[at(0x4000u, 0x231Cu)] = mode == 1u ? 0x10u :
                                mode == 2u ? 0x60u :
                                mode == 4u ? 0x61u : 0u;
    ram[at(0x4000u, 0x230Eu)] = mode == 1u ? 0u : 1u;
    put_word(ram, cpu.ds, 0x49D6u, 0x1000u);
    put_word(ram, cpu.ds, 0x49D8u, 0x5000u);
    put_word(ram, 0x5000u, 0x16E4u, 5u);
    status = pr_u5e_entry2_0123(ram, RAM_SIZE, &cpu, &io);
    if (without_callee) {
        assert(status == PR_U5E_0123_CALLEE_REQUIRED);
        assert(cpu.cs == 0xD100u && cpu.ip == 0x002Fu);
        assert(cpu.sp == 0x7FF2u && ram[at(cpu.ds, 0x681Du)] == 1u);
        assert(f.calls == 0u);
    } else {
        assert(status == PR_U5E_0123_RETURNED && f.calls == 1u);
        assert(cpu.bp == 0x7777u && cpu.sp == 0x8008u);
        assert(cpu.cs == 0x9876u && cpu.ip == 0x4567u);
        assert(cpu.ax == (uint16_t)(0xA500u |
                         (mode == 3u ? 0x77u : f.doubled)));
        assert(cpu.flags == 0xD246u);
        assert(ram[at(cpu.ds, 0x681Du)] == 0u);
        assert(ram[at(cpu.ds, 0x681Au)] ==
               (mode == 3u ? 0x77u : f.doubled));
    }
    free(ram);
}

int main(void)
{
    run(0u, 0);
    run(1u, 0);
    run(2u, 0);
    run(3u, 0);
    run(4u, 0);
    run(1u, 1);
    return 0;
}
