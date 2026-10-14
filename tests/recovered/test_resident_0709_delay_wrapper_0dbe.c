#include "poolrad/recovered/resident_0709_delay_wrapper_0dbe.h"

#include <stdio.h>
#include <string.h>

#define RAM_SIZE (1u << 20)
#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static uint8_t ram[RAM_SIZE];

static size_t address(uint16_t segment, uint16_t offset) {
    return (((size_t)segment << 4) + offset) & (RAM_SIZE - 1u);
}

static void set_word(uint16_t segment, uint16_t offset, uint16_t value) {
    size_t at = address(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get_word(uint16_t segment, uint16_t offset) {
    size_t at = address(segment, offset);
    return (uint16_t)(ram[at] |
                      ((uint16_t)ram[(at + 1u) & (RAM_SIZE - 1u)] << 8));
}

typedef struct {
    unsigned calls;
    uint16_t expected_argument;
    uint16_t expected_sp;
    int result;
    int corrupt_sp;
    int frame_ok;
} fixture;

static int a40(void *ctx, uint8_t *memory, size_t size,
               pr_r0709_delay_cpu *cpu, uint16_t argument) {
    fixture *f = ctx;
    ++f->calls;
    f->frame_ok = size == RAM_SIZE && memory == ram &&
                  argument == f->expected_argument &&
                  cpu->cs == 0x1264u && cpu->ip == 0x029Cu &&
                  cpu->sp == f->expected_sp &&
                  get_word(cpu->ss, cpu->sp) == 0x0DD1u &&
                  get_word(cpu->ss, (uint16_t)(cpu->sp + 2u)) == 0x0F2Du &&
                  get_word(cpu->ss, (uint16_t)(cpu->sp + 4u)) == argument &&
                  cpu->ax == argument && cpu->dx == 0u &&
                  (cpu->flags & 0x0801u) == 0u;
    cpu->ax = 0x5A5Au;
    cpu->bx = 0x1234u;
    cpu->cx = 0x2345u;
    cpu->dx = 0x3456u;
    cpu->si = 0x4567u;
    cpu->di = 0x5678u;
    cpu->bp = 0xBEEFu;
    cpu->es = 0x6789u;
    cpu->flags = 0x0246u;
    set_word(cpu->ss, (uint16_t)(cpu->sp - 2u), 0x0246u);
    if (f->corrupt_sp) cpu->sp = (uint16_t)(cpu->sp - 2u);
    return f->result;
}

static pr_r0709_delay_cpu initial(uint8_t factor, uint16_t sp) {
    pr_r0709_delay_cpu cpu = {
        0xABCDu, 0x2222u, 0x3333u, 0x4444u,
        0x5555u, 0x6666u, 0x7777u, sp,
        0x0F2Du, 0x0DBEu, 0x3000u, 0x8888u, 0xE000u, 0x7AD3u
    };
    memset(ram, 0xA5, sizeof ram);
    ram[address(cpu.ds, 0x49E2u)] = factor;
    set_word(cpu.ss, sp, 0x7135u);
    set_word(cpu.ss, (uint16_t)(sp + 2u), 0x9000u);
    return cpu;
}

static int successful_return(uint8_t factor, uint16_t sp) {
    pr_r0709_delay_cpu cpu = initial(factor, sp);
    pr_r0709_delay_report report;
    uint16_t argument = (uint16_t)(factor * 225u);
    fixture f = {0u, argument, (uint16_t)(sp - 8u), 0, 0, 0};

    CHECK(pr_r0709_delay_wrapper_0dbe(ram, sizeof ram, &cpu, 0x1264u,
                                       a40, &f, &report) == PR_R0709_DELAY_OK);
    CHECK(f.calls == 1u && f.frame_ok);
    CHECK(report.argument == argument && report.callback_invoked == 1u);
    CHECK(report.callee_frame_sp == (uint16_t)(sp - 8u));
    CHECK(cpu.ax == 0x5A5Au && cpu.bx == 0x1234u && cpu.cx == 0x2345u);
    CHECK(cpu.dx == 0x3456u && cpu.si == 0x4567u && cpu.di == 0x5678u);
    CHECK(cpu.bp == 0x7777u && cpu.es == 0x6789u && cpu.flags == 0x0246u);
    CHECK(cpu.sp == (uint16_t)(sp + 4u));
    CHECK(cpu.cs == 0x9000u && cpu.ip == 0x7135u);
    CHECK(get_word(cpu.ss, (uint16_t)(sp - 2u)) == 0x7777u);
    CHECK(get_word(cpu.ss, (uint16_t)(sp - 4u)) == argument);
    CHECK(get_word(cpu.ss, (uint16_t)(sp - 6u)) == 0x0F2Du);
    CHECK(get_word(cpu.ss, (uint16_t)(sp - 8u)) == 0x0DD1u);
    CHECK(get_word(cpu.ss, (uint16_t)(sp - 10u)) == 0x0246u);
    return 0;
}

static int boundaries(void) {
    pr_r0709_delay_cpu cpu = initial(4u, 0x8000u);
    pr_r0709_delay_report report;
    fixture f = {0u, 900u, 0x7FF8u, 7, 0, 0};

    CHECK(pr_r0709_delay_wrapper_0dbe(ram, sizeof ram, &cpu, 0x1264u,
                                       NULL, NULL, &report) ==
          PR_R0709_DELAY_EXTERNAL);
    CHECK(report.argument == 900u && report.callback_invoked == 0u);
    CHECK(cpu.cs == 0x1264u && cpu.ip == 0x029Cu && cpu.sp == 0x7FF8u);
    CHECK(cpu.bp == 0x7FFEu && get_word(cpu.ss, 0x7FFCu) == 900u);

    cpu = initial(4u, 0x8000u);
    CHECK(pr_r0709_delay_wrapper_0dbe(ram, sizeof ram, &cpu, 0x1264u,
                                       a40, &f, &report) ==
          PR_R0709_DELAY_CALLBACK);
    CHECK(f.calls == 1u && f.frame_ok && report.callback_status == 7);
    CHECK(cpu.sp == 0x7FF8u && cpu.cs == 0x1264u);

    cpu = initial(4u, 0x8000u);
    f.result = 0;
    f.corrupt_sp = 1;
    CHECK(pr_r0709_delay_wrapper_0dbe(ram, sizeof ram, &cpu, 0x1264u,
                                       a40, &f, &report) ==
          PR_R0709_DELAY_CALLEE_FRAME);
    CHECK(cpu.sp == 0x7FF6u && report.callback_invoked == 1u);

    cpu = initial(4u, 0x8000u);
    CHECK(pr_r0709_delay_wrapper_0dbe(NULL, RAM_SIZE, &cpu, 0x1264u,
                                       a40, &f, &report) ==
          PR_R0709_DELAY_ARGUMENT);
    CHECK(cpu.sp == 0x8000u && cpu.bp == 0x7777u);
    CHECK(pr_r0709_delay_wrapper_0dbe(ram, RAM_SIZE - 1u, &cpu, 0x1264u,
                                       a40, &f, &report) ==
          PR_R0709_DELAY_ARGUMENT);
    return 0;
}

int main(void) {
    if (successful_return(0u, 0x8000u)) return 1;
    if (successful_return(4u, 0x8000u)) return 1;
    if (successful_return(255u, 0x0002u)) return 1;
    if (boundaries()) return 1;
    puts("synthetic RES_0709:0DBE frame and callback passed");
    return 0;
}
