#include "poolrad/recovered/resident_loader_0538.h"

#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) {                                   \
    fprintf(stderr, "loader +0538 line %d: %s\n", __LINE__, #c); \
    return 1; } } while (0)

static uint8_t ram[PR_HANDLER_02E6_RAM_SIZE];
static uint8_t before[PR_HANDLER_02E6_RAM_SIZE];

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_HANDLER_02E6_RAM_SIZE - 1u);
}

static void put16(uint16_t seg, uint16_t off, uint16_t value)
{
    ram[at(seg, off)] = (uint8_t)value;
    ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(value >> 8);
}

static pr_handler_02e6_cpu frame(uint16_t selector, uint16_t direct,
                                  uint16_t fallback, uint16_t base)
{
    pr_handler_02e6_cpu cpu = {
        0x0047u, 0x0027u, 0x0004u, 0x0047u, 0x44BAu, 0x0000u,
        0x3794u, 0x3776u, 0x12C6u, 0x0538u, 0x14A0u, 0x08D0u,
        0x1D16u, 0x7206u
    };
    memset(ram, 0, sizeof ram);
    put16(cpu.ds, 0x43F6u, base);
    put16(cpu.ds, 0x43F8u, fallback);
    put16(cpu.ds, 0x43FAu, selector);
    if (selector != 0u)
        put16(selector, 0x0010u, direct);
    put16(cpu.ss, cpu.sp, 0x0366u);
    return cpu;
}

static int saved_direct(void)
{
    pr_handler_02e6_cpu cpu = frame(0x085Cu, 0x2DCFu, 0x3996u,
                                    0x2CCDu), expected = cpu;
    memcpy(before, ram, sizeof ram);
    CHECK(pr_resident_loader_0538(ram, sizeof ram, &cpu) ==
          PR_LOADER_0538_RETURNED);
    expected.ax = 0x0102u;
    expected.es = 0x085Cu;
    expected.ip = 0x0366u;
    expected.sp = 0x3778u;
    expected.flags = 0x7202u;
    CHECK(memcmp(&cpu, &expected, sizeof cpu) == 0);
    CHECK(memcmp(ram, before, sizeof ram) == 0);
    return 0;
}

static int null_selector(void)
{
    pr_handler_02e6_cpu cpu = frame(0u, 0u, 0x3996u, 0x2CCDu);
    CHECK(pr_resident_loader_0538(ram, sizeof ram, &cpu) ==
          PR_LOADER_0538_RETURNED);
    CHECK(cpu.ax == 0x0CC9u && cpu.es == 0x08D0u);
    CHECK(cpu.ip == 0x0366u && cpu.sp == 0x3778u);
    CHECK(cpu.flags == 0x7216u); /* Low byte C9 has even parity; AF set. */
    return 0;
}

static int borrowed_direct_uses_fallback(void)
{
    pr_handler_02e6_cpu cpu = frame(0x085Cu, 0x0001u, 0x3996u,
                                    0x2CCDu);
    CHECK(pr_resident_loader_0538(ram, sizeof ram, &cpu) ==
          PR_LOADER_0538_RETURNED);
    CHECK(cpu.ax == 0x0CC9u && cpu.es == 0x085Cu);
    CHECK(cpu.flags == 0x7216u); /* Final SUB, not first borrowed SUB. */
    return 0;
}

static int equality_takes_direct(void)
{
    pr_handler_02e6_cpu cpu = frame(0x085Cu, 0x2CCDu, 0x3996u,
                                    0x2CCDu);
    CHECK(pr_resident_loader_0538(ram, sizeof ram, &cpu) ==
          PR_LOADER_0538_RETURNED);
    CHECK(cpu.ax == 0u && cpu.flags == 0x7246u);
    return 0;
}

static int signed_overflow_does_not_force_fallback(void)
{
    pr_handler_02e6_cpu cpu = frame(0x085Cu, 0x8000u, 0x3996u,
                                    0x0001u);
    CHECK(pr_resident_loader_0538(ram, sizeof ram, &cpu) ==
          PR_LOADER_0538_RETURNED);
    CHECK(cpu.ax == 0x7FFFu && cpu.flags == 0x7A16u);
    return 0;
}

static int stack_word_crossing(void)
{
    pr_handler_02e6_cpu cpu = frame(0x085Cu, 0x2DCFu, 0x3996u,
                                    0x2CCDu);
    size_t next_segment = (((size_t)cpu.ss << 4) + 0x10000u) &
                          (PR_HANDLER_02E6_RAM_SIZE - 1u);
    cpu.sp = 0xFFFFu;
    put16(cpu.ss, cpu.sp, 0x1234u);
    ram[next_segment] = 0xA5u;
    CHECK(pr_resident_loader_0538(ram, sizeof ram, &cpu) ==
          PR_LOADER_0538_RETURNED);
    CHECK(cpu.ip == 0x1234u && cpu.sp == 0x0001u);
    CHECK(ram[next_segment] == 0xA5u);
    return 0;
}

static int arguments(void)
{
    pr_handler_02e6_cpu cpu = frame(0x085Cu, 0x2DCFu, 0x3996u,
                                    0x2CCDu), expected = cpu;
    memcpy(before, ram, sizeof ram);
    CHECK(pr_resident_loader_0538(NULL, sizeof ram, &cpu) ==
          PR_LOADER_0538_ARGUMENT);
    CHECK(pr_resident_loader_0538(ram, sizeof ram - 1u, &cpu) ==
          PR_LOADER_0538_ARGUMENT);
    CHECK(pr_resident_loader_0538(ram, sizeof ram, NULL) ==
          PR_LOADER_0538_ARGUMENT);
    CHECK(memcmp(&cpu, &expected, sizeof cpu) == 0);
    CHECK(memcmp(ram, before, sizeof ram) == 0);
    return 0;
}

int main(void)
{
    if (saved_direct() || null_selector() || borrowed_direct_uses_fallback() ||
        equality_takes_direct() || signed_overflow_does_not_force_fallback() ||
        stack_word_crossing() || arguments())
        return 1;
    puts("resident loader +0538 synthetic routes: 7 passed");
    return 0;
}
