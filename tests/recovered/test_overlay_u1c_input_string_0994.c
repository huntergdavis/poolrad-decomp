#include "poolrad/recovered/overlay_u1c_input_string_0994.h"

#include <assert.h>
#include <stdlib.h>

typedef struct {
    unsigned calls;
    unsigned abort_at;
    int empty;
    int mutate_return;
} fixture;

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U1C_0994_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t p = physical(seg, off);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U1C_0994_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t p = physical(seg, off);
    return (uint16_t)(ram[p] |
        ((uint16_t)ram[(p + 1u) & (PR_U1C_0994_RAM_SIZE - 1u)] << 8));
}

static int external(void *context, uint8_t *ram, size_t ram_size,
                    pr_u1c_0994_cpu *cpu, pr_u1c_0994_call call)
{
    fixture *f = context;
    uint16_t sp = cpu->sp;
    assert(ram_size == PR_U1C_0994_RAM_SIZE);
    ++f->calls;
    if (f->calls == 1u) {
        assert(call == PR_U1C_0994_LOADER && cpu->ip == 0x099Eu);
    } else if (f->calls == 2u) {
        assert(call == PR_U1C_0994_WORD && cpu->ip == 0x09AFu);
    } else if (f->calls == 3u) {
        assert(call == PR_U1C_0994_INPUT && cpu->ip == 0x09C8u);
    } else if (f->calls == 4u) {
        assert(call == PR_U1C_0994_COPY && cpu->ip == 0x09D7u);
    } else if (f->empty && f->calls == 5u) {
        assert(call == PR_U1C_0994_COPY && cpu->ip == 0x09F2u);
    } else {
        assert(call == PR_U1C_0994_STORE && cpu->ip == 0x0A00u);
    }
    if (f->calls == f->abort_at) return 0;
    if (call == PR_U1C_0994_LOADER) {
        assert(get16(ram, cpu->ss, sp) == 0xA502u);
        ram[physical(cpu->ds, 0x7087u)] = 0x12u;
        ram[physical(cpu->ds, 0x7047u)] = 0x34u;
        cpu->ax = 0xB100u;
    } else if (call == PR_U1C_0994_WORD) {
        assert(get16(ram, cpu->ss, sp) == 0xB134u);
        assert(get16(ram, cpu->ss, (uint16_t)(sp + 2u)) == 0xB112u);
        cpu->ax = 0x3412u;
    } else if (call == PR_U1C_0994_INPUT) {
        uint16_t dst_off = get16(ram, cpu->ss, (uint16_t)(sp + 8u));
        uint16_t dst_seg = get16(ram, cpu->ss, (uint16_t)(sp + 10u));
        assert(get16(ram, cpu->ss, sp) == 0x3428u);
        assert(get16(ram, cpu->ss, (uint16_t)(sp + 2u)) == 0x340Au);
        assert(dst_seg == cpu->ss && dst_off == (uint16_t)(cpu->bp - 0x0204u));
        ram[physical(dst_seg, dst_off)] = (uint8_t)(f->empty ? 0u : 1u);
        ram[physical(dst_seg, (uint16_t)(dst_off + 1u))] = 'A';
        cpu->ax = 0xCDEFu;
    } else if (call == PR_U1C_0994_COPY) {
        uint16_t dst_off = get16(ram, cpu->ss, (uint16_t)(sp + 2u));
        uint16_t dst_seg = get16(ram, cpu->ss, (uint16_t)(sp + 4u));
        uint16_t src_off = get16(ram, cpu->ss, (uint16_t)(sp + 6u));
        uint16_t src_seg = get16(ram, cpu->ss, (uint16_t)(sp + 8u));
        uint8_t length = ram[physical(src_seg, src_off)];
        assert(get16(ram, cpu->ss, sp) == 0x00FFu);
        assert(dst_seg == cpu->ss && dst_off == (uint16_t)(cpu->bp - 0x0104u));
        assert(length <= 1u);
        ram[physical(dst_seg, dst_off)] = length;
        if (length != 0u)
            ram[physical(dst_seg, (uint16_t)(dst_off + 1u))] =
                ram[physical(src_seg, (uint16_t)(src_off + 1u))];
    } else {
        uint16_t src_off = get16(ram, cpu->ss, sp);
        uint16_t src_seg = get16(ram, cpu->ss, (uint16_t)(sp + 2u));
        assert(src_seg == cpu->ss && src_off == (uint16_t)(cpu->bp - 0x0104u));
        assert(get16(ram, cpu->ss, (uint16_t)(sp + 4u)) == 0x3412u);
        assert(ram[physical(src_seg, src_off)] == 1u);
        assert(ram[physical(src_seg, (uint16_t)(src_off + 1u))] ==
               (uint8_t)(f->empty ? ' ' : 'A'));
        put16(ram, cpu->ds, 0x8460u, 0x2211u);
        if (f->mutate_return) {
            put16(ram, cpu->ss, (uint16_t)(sp + 0x020Au), 0x3344u);
            put16(ram, cpu->ss, (uint16_t)(sp + 0x020Cu), 0x7788u);
            put16(ram, cpu->ss, (uint16_t)(sp + 0x020Eu), 0x99AAu);
        }
    }
    return 1;
}

static void run(int empty, unsigned abort_at, int mutate_return)
{
    uint8_t *ram = calloc(PR_U1C_0994_RAM_SIZE, 1u);
    pr_u1c_0994_cpu cpu = {0};
    fixture f = {0u, abort_at, empty, mutate_return};
    static const uint16_t sites[] = {0u, 0x099Eu, 0x09AFu, 0x09C8u,
                                      0x09D7u, 0x09F2u, 0x0A00u};
    assert(ram != NULL);
    cpu.ax = 0xA5CDu; cpu.bp = 0x7777u; cpu.sp = 0x8000u;
    cpu.cs = 0xF000u; cpu.ip = 0x0994u;
    cpu.ds = 0x3000u; cpu.ss = 0xE000u;
    ram[physical(cpu.cs, 0x0992u)] = 1u;
    ram[physical(cpu.cs, 0x0993u)] = ' ';
    put16(ram, cpu.ss, cpu.sp, 0x0712u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    if (abort_at != 0u) {
        assert(pr_u1c_input_string_0994(ram, PR_U1C_0994_RAM_SIZE,
                                        &cpu, external, &f) == PR_U1C_0994_CALL_ABORT);
        assert(f.calls == abort_at);
        assert(cpu.ip == sites[abort_at == 5u && !empty ? 6u : abort_at]);
    } else {
        assert(pr_u1c_input_string_0994(ram, PR_U1C_0994_RAM_SIZE,
                                        &cpu, external, &f) == PR_U1C_0994_OK);
        assert(f.calls == (unsigned)(empty ? 6 : 5));
        assert(cpu.sp == 0x8004u);
        assert(get16(ram, cpu.ds, 0x8460u) == 0x2211u);
        assert(cpu.bp == (mutate_return ? 0x3344u : 0x7777u));
        assert(cpu.ip == (mutate_return ? 0x7788u : 0x0712u));
        assert(cpu.cs == (mutate_return ? 0x99AAu : 0xD000u));
    }
    free(ram);
}

int main(void)
{
    unsigned i;
    for (i = 0u; i <= 6u; ++i) run(1, i, 0);
    for (i = 0u; i <= 5u; ++i) run(0, i, 0);
    run(1, 0u, 1);
    run(0, 0u, 1);
    return 0;
}
