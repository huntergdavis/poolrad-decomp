#include "overlay_u86_entry1_postdc0_jge_fall_mov_al0_after_fourth_push.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE PR_1816_CALLER_RAM_SIZE
#define RAM_MASK (RAM_SIZE - 1u)

typedef struct {
    uint16_t ss, sp, bp, cs, ax, flags;
    uint8_t seed;
} fixture;

static size_t addr(uint16_t seg, uint16_t off) {
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t word) {
    ram[addr(seg, off)] = (uint8_t)word;
    ram[addr(seg, (uint16_t)(off + 1u))] = (uint8_t)(word >> 8);
}

static void check_rejected(uint8_t *ram, const uint8_t *expected,
                           pr_1816_caller_cpu *cpu,
                           pr_u86_entry1_postdc0_jge_fall_mov_al0_after_fourth_push_report *report,
                           size_t ram_size) {
    pr_1816_caller_cpu want = *cpu;
    unsigned i;
    memset(report, 0xCC, sizeof(*report));
    assert(pr_u86_entry1_postdc0_jge_fall_mov_al0_after_fourth_push(
               ram, ram_size, cpu, report) == 1);
    assert(memcmp(ram, expected, RAM_SIZE) == 0);
    assert(memcmp(cpu, &want, sizeof(*cpu)) == 0);
    for (i = 0u; i < sizeof(*report); ++i)
        assert(((const uint8_t *)report)[i] == 0xCCu);
}

static void check_one(const fixture *f, unsigned case_index) {
    uint8_t *ram = malloc(RAM_SIZE);
    uint8_t *expected = malloc(RAM_SIZE);
    pr_1816_caller_cpu cpu = {0}, want;
    pr_u86_entry1_postdc0_jge_fall_mov_al0_after_fourth_push_report report = {0};
    uint16_t high_ax = (uint16_t)(f->ax & 0xFF00u);
    uint16_t bad_off;
    assert(ram != NULL && expected != NULL);
    memset(ram, f->seed, RAM_SIZE);
    wr16(ram, f->ss, f->sp, f->ax);
    wr16(ram, f->ss, (uint16_t)(f->sp + 2u),
         (uint16_t)(high_ax | 0x26u));
    wr16(ram, f->ss, (uint16_t)(f->sp + 4u),
         (uint16_t)(high_ax | 3u));
    wr16(ram, f->ss, (uint16_t)(f->sp + 6u),
         (uint16_t)(high_ax | 1u));
    memcpy(expected, ram, RAM_SIZE);

    cpu.ax = f->ax;
    cpu.bx = 0x1357u;
    cpu.cx = 0x2468u;
    cpu.dx = 0xABCDu;
    cpu.si = 0x5678u;
    cpu.di = 0x9876u;
    cpu.bp = f->bp;
    cpu.sp = f->sp;
    cpu.cs = f->cs;
    cpu.ip = 0x015Eu;
    cpu.ds = 0xAAAAu;
    cpu.es = 0x5555u;
    cpu.ss = f->ss;
    cpu.flags = f->flags;
    want = cpu;
    want.ax = high_ax;
    want.ip = 0x0160u;

    cpu.ip = 0x015Du;
    check_rejected(ram, expected, &cpu, &report, RAM_SIZE);
    cpu.ip = 0x015Eu;
    cpu.flags = 0u;
    check_rejected(ram, expected, &cpu, &report, RAM_SIZE);
    cpu.flags = 0x0880u;
    check_rejected(ram, expected, &cpu, &report, RAM_SIZE);
    cpu.flags = f->flags;
    check_rejected(ram, expected, &cpu, &report, RAM_SIZE - 1u);
    bad_off = (uint16_t)(f->sp + 2u * (case_index % 4u));
    ram[addr(f->ss, bad_off)] ^= 0x80u;
    expected[addr(f->ss, bad_off)] ^= 0x80u;
    check_rejected(ram, expected, &cpu, &report, RAM_SIZE);
    ram[addr(f->ss, bad_off)] ^= 0x80u;
    expected[addr(f->ss, bad_off)] ^= 0x80u;

    assert(pr_u86_entry1_postdc0_jge_fall_mov_al0_after_fourth_push(
               ram, RAM_SIZE, &cpu, &report) == 0);
    assert(memcmp(ram, expected, RAM_SIZE) == 0);
    assert(memcmp(&cpu, &want, sizeof(cpu)) == 0);
    assert(report.before_ax == f->ax && report.stop_ax == high_ax);
    assert(report.before_flags == f->flags && report.stop_ip == 0x0160u);
    free(expected);
    free(ram);
}

int main(void) {
    static const fixture cases[8] = {
        {0x1D16u, 0x32F8u, 0x3400u, 0x3456u, 0xA57Fu, 0x0084u, 0xA5u},
        {0x1000u, 0xFFF8u, 0x4000u, 0x2000u, 0x0000u, 0x0080u, 0x00u},
        {0xFFFFu, 0x000Fu, 0x0031u, 0x1111u, 0x7F26u, 0x0080u, 0xFFu},
        {0xFFFFu, 0x0010u, 0x005Eu, 0x2222u, 0x80FFu, 0x0281u, 0x33u},
        {0x0000u, 0xFFFDu, 0x0006u, 0x7777u, 0xFF03u, 0x0080u, 0x5Au},
        {0xF000u, 0xFFFFu, 0x8000u, 0x3333u, 0x0126u, 0x0800u, 0x7Cu},
        {0xA000u, 0x7FF8u, 0xFFF6u, 0x4444u, 0x5501u, 0x0281u, 0x22u},
        {0x8000u, 0x0000u, 0xFFFDu, 0x5555u, 0x1200u, 0x0080u, 0x11u},
    };
    unsigned i;
    for (i = 0u; i < 8u; ++i)
        check_one(&cases[i], i);
    puts("U86 +015E post-fourth-PUSH MOV AL,0 synthetic C: 8/8 PASS");
    return 0;
}
