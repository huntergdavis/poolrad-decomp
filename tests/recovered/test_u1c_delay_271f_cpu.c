#include "poolrad/recovered/overlay_u1c_delay_271f.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); exit(1); } } while (0)
#define SIZE PR_R0709_DELAY_CPU_RAM_SIZE
static uint8_t ram[SIZE], expected[SIZE];

static size_t physical(uint16_t s, uint16_t o)
{
    return (((size_t)s << 4) + o) & (SIZE - 1u);
}

static void word(uint16_t s, uint16_t o, uint16_t value)
{
    size_t a = physical(s, o);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t readword(uint16_t s, uint16_t o)
{
    size_t a = physical(s, o);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & (SIZE - 1u)] << 8));
}

static pr_u1c_delay_cpu setup(unsigned factor, unsigned count, unsigned pc)
{
    pr_u1c_delay_cpu cpu = {0x1234, 0x2345, 0x3456, 0x4567, 0x5678, 0x6789, 0x789A,
                            0x8000, 0x9000, 0x271F, 0x3000, 0x2222, 0x4000, 0x0603};
    memset(ram, 0xA5, sizeof ram);
    /* Synthetic call operands only: no original executable bytes. */
    ram[physical(cpu.cs, 0x2726)] = 0x9A;
    word(cpu.cs, 0x2727, 0x0DBE);
    word(cpu.cs, 0x2729, 0x0709);
    ram[physical(0x0709, 0x0DCC)] = 0x9A;
    word(0x0709, 0x0DCD, 0x029C);
    word(0x0709, 0x0DCF, 0x0A40);
    word(cpu.ds, 0x49ED, (uint16_t)pc);
    word(cpu.ds, 0x84FC, (uint16_t)count);
    ram[physical(cpu.ds, 0x49E2)] = (uint8_t)factor;
    word(cpu.ss, cpu.sp, 0x7000);
    word(cpu.ss, (uint16_t)(cpu.sp + 2u), 0x9200);
    return cpu;
}

static void arguments(void)
{
    pr_u1c_delay_cpu cpu = setup(1, 1, 1), before = cpu;
    pr_u1c_delay_cpu_report r = {17, 18};
    memcpy(expected, ram, SIZE);
    CHECK(pr_u1c_delay_271f_cpu(NULL, SIZE, &cpu, 0, &r) == PR_U1C_DELAY_CPU_ARGUMENT);
    CHECK(r.wrapper_status == -1 && r.delay_status == -1);
    CHECK(pr_u1c_delay_271f_cpu(ram, SIZE, NULL, 0, &r) == PR_U1C_DELAY_CPU_ARGUMENT);
    CHECK(pr_u1c_delay_271f_cpu(ram, SIZE - 1u, &cpu, 0, &r) == PR_U1C_DELAY_CPU_ARGUMENT);
    CHECK(!memcmp(&cpu, &before, sizeof cpu));
    cpu.ip++;
    before = cpu;
    CHECK(pr_u1c_delay_271f_cpu(ram, SIZE, &cpu, 0, &r) == PR_U1C_DELAY_CPU_ARGUMENT);
    CHECK(!memcmp(&cpu, &before, sizeof cpu));
    cpu.ip = 0x271F;
    for (unsigned i = 0; i < 2; ++i) {
        cpu.sp = i ? 0xFFFD : 0x000F;
        before = cpu;
        CHECK(pr_u1c_delay_271f_cpu(ram, SIZE, &cpu, 0, &r) == PR_U1C_DELAY_CPU_ARGUMENT);
        CHECK(!memcmp(&cpu, &before, sizeof cpu));
    }
    CHECK(!memcmp(ram, expected, SIZE));
}

static void increment_guard(void)
{
    pr_u1c_delay_cpu initial = setup(1, 1, 0), cpu;
    pr_u1c_delay_cpu_report r;
    ram[physical(initial.cs, 0x2726)] = 0x90;
    for (unsigned carry = 0; carry < 2; ++carry) {
        for (unsigned pc = 0; pc < 65536; ++pc) {
            unsigned result = (pc + 1u) & 65535u, bits = 0;
            uint16_t want = (uint16_t)(0x7202u | carry);
            for (unsigned b = 0; b < 8; ++b) bits += (result >> b) & 1u;
            if (!(bits & 1u)) want |= 4;
            if (!result) want |= 0x40;
            if (result >= 32768) want |= 0x80;
            if ((pc & 15u) == 15) want |= 0x10;
            if (pc == 32767) want |= 0x800;
            cpu = initial;
            cpu.flags = (uint16_t)(0x7AD6u | carry); /* Arithmetic flags begin set. */
            word(cpu.ds, 0x49ED, (uint16_t)pc);
            CHECK(pr_u1c_delay_271f_cpu(ram, SIZE, &cpu, 0, &r) == PR_U1C_DELAY_CPU_CODE);
            CHECK(cpu.flags == want);
            CHECK(readword(cpu.ds, 0x49ED) == result);
            CHECK(cpu.ip == 0x2726 && cpu.cs == initial.cs);
            CHECK(cpu.bp == 0x7FFE && cpu.sp == 0x7FFE && readword(cpu.ss, cpu.sp) == initial.bp);
            CHECK(cpu.ax == initial.ax && cpu.bx == initial.bx && cpu.cx == initial.cx && cpu.dx == initial.dx);
            CHECK(cpu.si == initial.si && cpu.di == initial.di && cpu.ds == initial.ds && cpu.es == initial.es && cpu.ss == initial.ss);
            CHECK(r.wrapper_status == -1 && r.delay_status == -1);
        }
    }
    cpu = setup(1, 1, 0xFFFF);
    ram[physical(cpu.cs, 0x2727)] = 0;
    memcpy(expected, ram, SIZE);
    /* At an outer guard only INC and PUSH BP may write memory. */
    size_t a = physical(cpu.ds, 0x49ED);
    expected[a] = expected[a + 1] = 0;
    a = physical(cpu.ss, 0x7FFE);
    expected[a] = (uint8_t)cpu.bp; expected[a + 1] = (uint8_t)(cpu.bp >> 8);
    CHECK(pr_u1c_delay_271f_cpu(ram, SIZE, &cpu, 0, NULL) == PR_U1C_DELAY_CPU_CODE);
    CHECK(!memcmp(ram, expected, SIZE));
}

static void nested_guard(void)
{
    for (unsigned factor = 0; factor < 256; ++factor) {
        pr_u1c_delay_cpu cpu = setup(factor, 1, 0xFFFF);
        pr_u1c_delay_cpu_report r;
        ram[physical(0x0709, 0x0DCC)] = 0x90;
        CHECK(pr_u1c_delay_271f_cpu(ram, SIZE, &cpu, 0, &r) == PR_U1C_DELAY_CPU_CALLEE);
        CHECK(r.wrapper_status == PR_R0709_DELAY_CPU_CODE && r.delay_status == -1);
        CHECK(cpu.cs == 0x0709 && cpu.ip == 0x0DCC && cpu.sp == 0x7FF6 && cpu.bp == 0x7FF8);
        CHECK(cpu.ax == factor * 225u && cpu.dx == 0 && readword(cpu.ss, cpu.sp) == factor * 225u);
        CHECK(readword(cpu.ds, 0x49ED) == 0);
        CHECK(readword(cpu.ss, 0x7FFA) == 0x272B && readword(cpu.ss, 0x7FFC) == 0x9000);
    }
}

static void budgets_and_returns(void)
{
    static const struct { unsigned count, cap, ip, bx, cx, dx; } cases[] = {
        {1, 0, 0x02C0, 3, 1, 225},
        {1, 2, 0x02C0, 1, 1, 225},
        {1, 3, 0x02B0, 0, 0, 224},
        {2, 3, 0x02BD, 0, 1, 225},
        {0, 3, 0x02BD, 0, 65535, 225}
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        pr_u1c_delay_cpu cpu = setup(1, cases[i].count, 0xFFFF);
        pr_u1c_delay_cpu_report r;
        CHECK(pr_u1c_delay_271f_cpu(ram, SIZE, &cpu, cases[i].cap, &r) == PR_U1C_DELAY_CPU_CALLEE);
        CHECK(r.wrapper_status == PR_R0709_DELAY_CPU_CALLEE && r.delay_status == 2);
        CHECK(cpu.cs == 0x0A40 && cpu.ip == cases[i].ip);
        CHECK(cpu.bx == cases[i].bx && cpu.cx == cases[i].cx && cpu.dx == cases[i].dx);
        CHECK(cpu.sp == (cases[i].ip == 0x02B0 ? 0x7FF2 : 0x7FF0));
        CHECK(readword(cpu.ds, 0x49ED) == 0);
        CHECK(readword(cpu.ss, 0x7FF0) == 0x02B7 && readword(cpu.ss, 0x7FF2) == 0x0DD1);
    }
    for (unsigned factor = 0; factor < 256; ++factor) {
        pr_u1c_delay_cpu cpu = setup(factor, 1, 0xFFFF);
        pr_u1c_delay_cpu_report r;
        CHECK(pr_u1c_delay_271f_cpu(ram, SIZE, &cpu, 300000, &r) == PR_U1C_DELAY_CPU_OK);
        CHECK(r.wrapper_status == 0 && r.delay_status == 0);
        CHECK(cpu.cs == 0x9200 && cpu.ip == 0x7000 && cpu.sp == 0x8004 && cpu.bp == 0x789A);
        CHECK(readword(cpu.ds, 0x49ED) == 0 && cpu.si == 0x5678);
        CHECK(cpu.dx == 0 && cpu.flags == 0x0646);
        CHECK(cpu.ds == 0x3000 && cpu.ss == 0x4000);
        if (factor) CHECK(cpu.es == 0x0040 && cpu.di == 0 && cpu.cx == 0 && cpu.bx == 0);
        else CHECK(cpu.es == 0x2222 && cpu.di == 0x6789 && cpu.cx == 0x3456 && cpu.bx == 0x7FF2);
    }
}

int main(void)
{
    arguments();
    increment_guard();
    nested_guard();
    budgets_and_returns();
    printf("U1C 271F raw CPU: %u checks passed\n", checks);
    return 0;
}
