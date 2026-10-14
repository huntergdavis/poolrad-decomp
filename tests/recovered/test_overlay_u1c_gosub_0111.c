#include "poolrad/recovered/overlay_u1c_gosub_0111.h"

#include <assert.h>
#include <stdlib.h>

typedef struct {
    unsigned calls;
    unsigned abort_at;
    int mutate_return;
} fixture;

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U1C_0111_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t p = physical(seg, off);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U1C_0111_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t p = physical(seg, off);
    return (uint16_t)(ram[p] |
        ((uint16_t)ram[(p + 1u) & (PR_U1C_0111_RAM_SIZE - 1u)] << 8));
}

static int external(void *context, uint8_t *ram, size_t ram_size,
                    pr_u1c_0111_cpu *cpu, pr_u1c_0111_call call)
{
    fixture *f = context;
    assert(ram_size == PR_U1C_0111_RAM_SIZE);
    assert(call == f->calls);
    assert(get16(ram, cpu->ss, cpu->sp) == (call == 0 ? 0xA501u : 0xB101u));
    ++f->calls;
    if (f->calls == f->abort_at) return 0;
    if (call == PR_U1C_0111_LOADER) {
        put16(ram, cpu->ds, 0x49EDu, 0x2357u);
        cpu->ax = 0xB100u;
    } else {
        put16(ram, cpu->ds, 0x49EDu, 0x4567u);
        put16(ram, cpu->ds, 0x6F70u, 0x2468u);
        put16(ram, cpu->ds, 0x6F72u, 0x1357u);
        if (f->mutate_return) {
            put16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u), 0x3344u);
            put16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u), 0x7788u);
            put16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u), 0x99AAu);
        }
    }
    return 1;
}

static void run(unsigned abort_at, int mutate_return)
{
    uint8_t *ram = calloc(PR_U1C_0111_RAM_SIZE, 1u);
    pr_u1c_0111_cpu cpu = {0};
    fixture f = {0u, abort_at, mutate_return};
    assert(ram != NULL);
    cpu.ax = 0xA5CDu; cpu.bp = 0x7777u; cpu.sp = 0x8000u;
    cpu.cs = 0xF000u; cpu.ip = 0x0111u;
    cpu.ds = 0x3000u; cpu.ss = 0xE000u;
    put16(ram, cpu.ss, cpu.sp, 0x0712u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    if (abort_at != 0u) {
        assert(pr_u1c_gosub_0111(ram, PR_U1C_0111_RAM_SIZE,
                                 &cpu, external, &f) == PR_U1C_0111_CALL_ABORT);
        assert(f.calls == abort_at);
        assert(cpu.ip == (abort_at == 1u ? 0x0117u : 0x011Fu));
    } else {
        assert(pr_u1c_gosub_0111(ram, PR_U1C_0111_RAM_SIZE,
                                 &cpu, external, &f) == PR_U1C_0111_OK);
        assert(f.calls == 2u && cpu.sp == 0x8004u);
        assert(get16(ram, cpu.ds, 0x49EDu) == 0x4567u);
        assert(get16(ram, cpu.ds, 0x6F70u) == 0x2468u);
        assert(get16(ram, cpu.ds, 0x6F72u) == 0x1357u);
        assert(cpu.bp == (mutate_return ? 0x3344u : 0x7777u));
        assert(cpu.ip == (mutate_return ? 0x7788u : 0x0712u));
        assert(cpu.cs == (mutate_return ? 0x99AAu : 0xD000u));
    }
    free(ram);
}

int main(void)
{
    run(0u, 0);
    run(0u, 1);
    run(1u, 0);
    run(2u, 0);
    return 0;
}
