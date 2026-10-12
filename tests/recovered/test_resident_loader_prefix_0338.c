#include "poolrad/recovered/resident_loader_prefix_0338.h"

#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) {                                      \
    fprintf(stderr, "loader +0338 line %d: %s\n", __LINE__, #c);    \
    return 1; } } while (0)

static uint8_t ram[PR_HANDLER_02E6_RAM_SIZE];

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_HANDLER_02E6_RAM_SIZE - 1u);
}

static void put16(uint16_t seg, uint16_t off, uint16_t value)
{
    ram[at(seg, off)] = (uint8_t)value;
    ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(value >> 8);
}

static uint16_t get16(uint16_t seg, uint16_t off)
{
    return (uint16_t)(ram[at(seg, off)] |
                      ((uint16_t)ram[at(seg, (uint16_t)(off + 1u))] << 8));
}

static pr_handler_02e6_cpu frame(void)
{
    pr_handler_02e6_cpu cpu = {
        0x37AEu, 0x0027u, 0x0007u, 0x449Au, 0x44BAu, 0x0000u,
        0x3794u, 0x377Au, 0x12C6u, 0x0338u, 0x14A0u, 0x08D0u,
        0x1D16u, 0x7212u
    };
    memset(ram, 0, sizeof ram);
    put16(cpu.ds, 0x43E0u, 0x0029u);
    put16(cpu.ds, 0x43E2u, 0x0029u);
    put16(cpu.es, 0x0010u, 0u);
    put16(cpu.es, 0x0012u, 0u);
    put16(cpu.ss, cpu.sp, 0x0317u);
    return cpu;
}

static int saved_zero_branch(void)
{
    pr_handler_02e6_cpu cpu = frame(), expected = cpu;
    CHECK(pr_resident_loader_prefix_0338(ram, sizeof ram, &cpu) ==
          PR_LOADER_0338_CALL_0553);
    expected.ip = 0x0352u;
    expected.sp = 0x3778u;
    expected.flags = 0x7202u;
    CHECK(memcmp(&cpu, &expected, sizeof cpu) == 0);
    CHECK(get16(cpu.ds, 0x43E0u) == 0x002Au);
    CHECK(get16(cpu.ds, 0x43E2u) == 0x002Au);
    CHECK(get16(cpu.es, 0x0012u) == 0u);
    CHECK(get16(cpu.ss, cpu.sp) == cpu.es);
    CHECK(get16(cpu.ss, 0x377Au) == 0x0317u);
    return 0;
}

static int nonzero_branch(void)
{
    pr_handler_02e6_cpu cpu = frame(), expected = cpu;
    put16(cpu.es, 0x0010u, 1u);
    CHECK(pr_resident_loader_prefix_0338(ram, sizeof ram, &cpu) ==
          PR_LOADER_0338_CALL_0435);
    expected.ip = 0x03B6u;
    expected.flags = 0x7202u;
    CHECK(memcmp(&cpu, &expected, sizeof cpu) == 0);
    CHECK(get16(cpu.ds, 0x43E0u) == 0x002Au);
    CHECK(get16(cpu.ds, 0x43E2u) == 0x0029u);
    CHECK(get16(cpu.es, 0x0012u) == 1u);
    CHECK(get16(cpu.ss, cpu.sp) == 0x0317u);
    return 0;
}

static int second_inc_overflow(void)
{
    pr_handler_02e6_cpu cpu = frame();
    put16(cpu.ds, 0x43E2u, 0x7FFFu);
    CHECK(pr_resident_loader_prefix_0338(ram, sizeof ram, &cpu) ==
          PR_LOADER_0338_CALL_0553);
    CHECK(get16(cpu.ds, 0x43E2u) == 0x8000u);
    CHECK(cpu.flags == 0x7A96u); /* OF, SF, AF, PF, IF */
    return 0;
}

static int odd_stack_word_crossing(void)
{
    pr_handler_02e6_cpu cpu = frame();
    size_t next_segment = (((size_t)cpu.ss << 4) + 0x10000u) &
                          (PR_HANDLER_02E6_RAM_SIZE - 1u);
    cpu.sp = 0x0001u;
    ram[next_segment] = 0xA5u;
    CHECK(pr_resident_loader_prefix_0338(ram, sizeof ram, &cpu) ==
          PR_LOADER_0338_CALL_0553);
    CHECK(cpu.sp == 0xFFFFu && cpu.ip == 0x0352u);
    CHECK(ram[at(cpu.ss, 0xFFFFu)] == 0xD0u);
    CHECK(ram[at(cpu.ss, 0x0000u)] == 0x08u);
    CHECK(ram[next_segment] == 0xA5u);
    return 0;
}

static int argument_guards(void)
{
    pr_handler_02e6_cpu cpu = frame(), before = cpu;
    CHECK(pr_resident_loader_prefix_0338(NULL, sizeof ram, &cpu) ==
          PR_LOADER_0338_ARGUMENT);
    CHECK(pr_resident_loader_prefix_0338(ram, sizeof ram - 1u, &cpu) ==
          PR_LOADER_0338_ARGUMENT);
    CHECK(pr_resident_loader_prefix_0338(ram, sizeof ram, NULL) ==
          PR_LOADER_0338_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
    CHECK(get16(cpu.ds, 0x43E0u) == 0x0029u);
    return 0;
}

int main(void)
{
    if (saved_zero_branch() || nonzero_branch() || second_inc_overflow() ||
        odd_stack_word_crossing() || argument_guards())
        return 1;
    puts("resident loader +0338 first-call synthetic routes: 5 passed");
    return 0;
}
