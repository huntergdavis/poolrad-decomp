#include "overlay_u86_entry1_postdc0_joined_push_ss.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE PR_1816_CALLER_RAM_SIZE
#define RAM_MASK (RAM_SIZE - 1u)

typedef struct {
    uint16_t ss, bp, sp, cs, flags;
    uint8_t seed;
} fixture;

static size_t addr(uint16_t seg, uint16_t off) {
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t word) {
    ram[addr(seg, off)] = (uint8_t)word;
    ram[addr(seg, (uint16_t)(off + 1u))] = (uint8_t)(word >> 8);
}

static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off) {
    return (uint16_t)(ram[addr(seg, off)] |
        ((uint16_t)ram[addr(seg, (uint16_t)(off + 1u))] << 8));
}

static void check_rejected(uint8_t *ram, const uint8_t *expected,
                           pr_1816_caller_cpu *cpu,
                           pr_u86_entry1_postdc0_joined_push_ss_report *report,
                           size_t ram_size) {
    pr_1816_caller_cpu want = *cpu;
    unsigned i;
    memset(report, 0xCC, sizeof(*report));
    assert(pr_u86_entry1_postdc0_joined_push_ss(
               ram, ram_size, cpu, report) == 1);
    assert(memcmp(ram, expected, RAM_SIZE) == 0);
    assert(memcmp(cpu, &want, sizeof(*cpu)) == 0);
    for (i = 0u; i < sizeof(*report); ++i)
        assert(((const uint8_t *)report)[i] == 0xCCu);
}

static void check_one(const fixture *f) {
    uint8_t *ram = malloc(RAM_SIZE);
    uint8_t *expected = malloc(RAM_SIZE);
    pr_1816_caller_cpu cpu = {0}, want;
    pr_u86_entry1_postdc0_joined_push_ss_report report = {0};
    uint16_t first = (uint16_t)(f->bp - 5u);
    uint16_t second = (uint16_t)(f->bp - 3u);
    uint16_t stop_sp = (uint16_t)(f->sp - 2u);
    assert(ram != NULL && expected != NULL);
    memset(ram, f->seed, RAM_SIZE);
    ram[addr(f->ss, stop_sp)] = 0xA5u;
    ram[addr(f->ss, (uint16_t)(stop_sp + 1u))] = 0x5Au;
    wr16(ram, f->ss, first, 0u);
    wr16(ram, f->ss, second, 0u);
    memcpy(expected, ram, RAM_SIZE);

    cpu.ax = 0u;
    cpu.bx = 0x1357u;
    cpu.cx = 0x2468u;
    cpu.dx = 0xABCDu;
    cpu.si = 0x5678u;
    cpu.di = (uint16_t)(f->bp - 0x32u);
    cpu.bp = f->bp;
    cpu.sp = f->sp;
    cpu.cs = f->cs;
    cpu.ip = 0x019Fu;
    cpu.ds = 0xAAAAu;
    cpu.es = 0x5555u;
    cpu.ss = f->ss;
    cpu.flags = f->flags;
    want = cpu;
    want.sp = stop_sp;
    want.ip = 0x01A0u;

    cpu.ip = 0x019Cu;
    check_rejected(ram, expected, &cpu, &report, RAM_SIZE);
    cpu.ip = 0x019Fu;
    cpu.di ^= 1u;
    check_rejected(ram, expected, &cpu, &report, RAM_SIZE);
    cpu.di ^= 1u;
    cpu.ax = 1u;
    check_rejected(ram, expected, &cpu, &report, RAM_SIZE);
    cpu.ax = 0u;
    cpu.flags ^= 1u;
    check_rejected(ram, expected, &cpu, &report, RAM_SIZE);
    cpu.flags = f->flags;
    check_rejected(ram, expected, &cpu, &report, RAM_SIZE - 1u);
    ram[addr(f->ss, first)] ^= 0x80u;
    expected[addr(f->ss, first)] ^= 0x80u;
    check_rejected(ram, expected, &cpu, &report, RAM_SIZE);
    ram[addr(f->ss, first)] ^= 0x80u;
    expected[addr(f->ss, first)] ^= 0x80u;
    ram[addr(f->ss, second)] ^= 0x40u;
    expected[addr(f->ss, second)] ^= 0x40u;
    check_rejected(ram, expected, &cpu, &report, RAM_SIZE);
    ram[addr(f->ss, second)] ^= 0x40u;
    expected[addr(f->ss, second)] ^= 0x40u;
    assert(pr_u86_entry1_postdc0_joined_push_ss(
               ram, RAM_SIZE, &cpu, &report) == 0);
    wr16(expected, f->ss, stop_sp, f->ss);
    assert(memcmp(ram, expected, RAM_SIZE) == 0);
    assert(memcmp(&cpu, &want, sizeof(cpu)) == 0);
    assert(report.before_sp == f->sp && report.stop_sp == stop_sp);
    assert(report.pushed_ss == f->ss && report.stop_ip == 0x01A0u);
    assert(rd16(ram, f->ss, stop_sp) == f->ss);
    free(expected);
    free(ram);
}

int main(void) {
    static const fixture cases[8] = {
        {0x1D16u, 0x3400u, 0x3300u, 0x3456u, 0x0044u, 0xA5u},
        {0x1000u, 0x4000u, 0x0001u, 0x2000u, 0x0054u, 0x00u},
        {0xFFFFu, 0x0012u, 0x0011u, 0x1111u, 0x0254u, 0xFFu},
        {0xFFFFu, 0x0013u, 0x0012u, 0x2222u, 0x0046u, 0x33u},
        {0x0000u, 0x0005u, 0x0002u, 0x7777u, 0x0044u, 0x5Au},
        {0xF000u, 0x0003u, 0x0002u, 0x3333u, 0x0246u, 0x7Cu},
        {0xA000u, 0xFFFFu, 0xFFFEu, 0x4444u, 0x1044u, 0x22u},
        {0x8000u, 0xFFFDu, 0xFFFBu, 0x5555u, 0x1254u, 0x11u},
    };
    unsigned i;
    for (i = 0u; i < 8u; ++i)
        check_one(&cases[i]);
    puts("U86 +019F joined PUSH SS synthetic C: 8/8 PASS");
    return 0;
}
