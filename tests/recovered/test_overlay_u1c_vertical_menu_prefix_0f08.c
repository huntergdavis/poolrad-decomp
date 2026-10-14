#include "poolrad/recovered/overlay_u1c_vertical_menu_prefix_0f08.h"

#include <assert.h>
#include <stdlib.h>

typedef struct {
    unsigned calls;
    unsigned abort_at;
    int touch;
} fixture;

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U1C_0F08_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t p = physical(seg, off);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U1C_0F08_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t p = physical(seg, off);
    return (uint16_t)(ram[p] |
        ((uint16_t)ram[(p + 1u) & (PR_U1C_0F08_RAM_SIZE - 1u)] << 8));
}

static int external(void *context, uint8_t *ram, size_t ram_size,
                    pr_u1c_0f08_cpu *cpu, pr_u1c_0f08_call call)
{
    fixture *f = context;
    uint16_t sp = cpu->sp;
    assert(ram_size == PR_U1C_0F08_RAM_SIZE);
    ++f->calls;
    if (f->calls == f->abort_at) return 0;
    switch (f->calls) {
    case 1u:
        assert(call == PR_U1C_0F08_LOADER && cpu->ip == 0x0F25u);
        assert(get16(ram, cpu->ss, sp) == 0xA503u);
        ram[physical(cpu->ds, 0x7086u)] = 0x12u;
        ram[physical(cpu->ds, 0x7046u)] = 0x34u;
        put16(ram, cpu->ds, 0x49EDu, 1u);
        cpu->ax = 0xB100u;
        break;
    case 2u:
        assert(call == PR_U1C_0F08_WORD && cpu->ip == 0x0F32u);
        assert(get16(ram, cpu->ss, sp) == 0xB134u);
        assert(get16(ram, cpu->ss, (uint16_t)(sp + 2u)) == 0xB112u);
        cpu->ax = 0x47B2u;
        break;
    case 3u:
        assert(call == PR_U1C_0F08_COPY && cpu->ip == 0x0F4Au);
        assert(get16(ram, cpu->ss, sp) == 0x00FFu);
        assert(get16(ram, cpu->ss, (uint16_t)(sp + 2u)) ==
               (uint16_t)(cpu->bp - 0x0102u));
        assert(get16(ram, cpu->ss, (uint16_t)(sp + 4u)) == cpu->ss);
        assert(get16(ram, cpu->ss, (uint16_t)(sp + 6u)) == 0x70C8u);
        assert(get16(ram, cpu->ss, (uint16_t)(sp + 8u)) == cpu->ds);
        ram[physical(cpu->ss, (uint16_t)(cpu->bp - 0x0102u))] = 1u;
        ram[physical(cpu->ss, (uint16_t)(cpu->bp - 0x0101u))] = 'A';
        break;
    case 4u:
        assert(call == PR_U1C_0F08_VALUE && cpu->ip == 0x0F52u);
        assert(get16(ram, cpu->ss, sp) == 3u);
        cpu->ax = 0xA505u;
        break;
    default:
        assert(f->calls == 5u && call == PR_U1C_0F08_LOADER);
        assert(cpu->ip == 0x0F62u && get16(ram, cpu->ss, sp) == 0xA505u);
        assert(get16(ram, cpu->ds, 0x49EDu) == 0u);
        if (f->touch)
            ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1u))] = 0x52u;
        cpu->ax = 0xBD20u;
        break;
    }
    return 1;
}

static void run(unsigned abort_at, int touch)
{
    uint8_t *ram = calloc(PR_U1C_0F08_RAM_SIZE, 1u);
    pr_u1c_0f08_cpu cpu = {0};
    fixture f = {0u, abort_at, touch};
    static const uint16_t sites[] = {0u, 0x0F25u, 0x0F32u,
                                      0x0F4Au, 0x0F52u, 0x0F62u};
    assert(ram != NULL);
    cpu.ax = 0xA5CDu; cpu.bp = 0x7777u; cpu.sp = 0x8000u;
    cpu.cs = 0xF000u; cpu.ip = 0x0F08u;
    cpu.ds = 0x3000u; cpu.ss = 0xE000u;
    ram[physical(cpu.ds, 0x70C8u)] = 1u;
    ram[physical(cpu.ds, 0x70C9u)] = 'A';
    put16(ram, cpu.ss, cpu.sp, 0x0712u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    if (abort_at != 0u) {
        assert(pr_u1c_vertical_menu_prefix_0f08(ram, PR_U1C_0F08_RAM_SIZE,
                                                  &cpu, external, &f) ==
               PR_U1C_0F08_CALL_ABORT);
        assert(f.calls == abort_at && cpu.ip == sites[abort_at]);
    } else {
        assert(pr_u1c_vertical_menu_prefix_0f08(ram, PR_U1C_0F08_RAM_SIZE,
                                                  &cpu, external, &f) ==
               PR_U1C_0F08_PREFIX_DONE);
        assert(f.calls == 5u);
        assert(cpu.ip == 0x0F73u && cpu.cs == 0xF000u);
        assert(cpu.bp == 0x7FFEu && cpu.sp == 0x7EE6u);
        assert(cpu.di == 0x7EF8u);
        assert(get16(ram, cpu.ss, cpu.sp) == cpu.di);
        assert(get16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u)) == cpu.ss);
        assert(get16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u)) ==
               (uint16_t)(touch ? 0x52u : 5u));
        assert(get16(ram, cpu.ds, 0x49EDu) == 0u);
        assert(ram[physical(cpu.ds, 0x84E4u)] == 0u);
        assert(ram[physical(cpu.ss, 0x7EFCu)] == 1u);
    }
    free(ram);
}

int main(void)
{
    unsigned i;
    for (i = 0u; i <= 5u; ++i) run(i, 0);
    run(0u, 1);
    return 0;
}
