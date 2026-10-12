#include "poolrad/recovered/resident_loader_0355.h"

#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) {                                   \
    fprintf(stderr, "loader +0355 line %d: %s\n", __LINE__, #c); \
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

static pr_handler_02e6_cpu frame(uint16_t size, uint16_t ax)
{
    pr_handler_02e6_cpu cpu = {
        ax, 0x0027u, 0x0004u, 0x449Au, 0x44BAu, 0x0000u,
        0x3794u, 0x3778u, 0x12C6u, 0x0355u, 0x14A0u, 0x08D0u,
        0x1D16u, 0x7216u
    };
    memset(ram, 0, sizeof ram);
    put16(cpu.es, 0x000Au, size);
    return cpu;
}

static int saved_zero(void)
{
    pr_handler_02e6_cpu cpu = frame(0u, 0x0047u), expected = cpu;
    uint8_t before[sizeof ram];
    memcpy(before, ram, sizeof ram);
    CHECK(pr_resident_loader_0355(ram, sizeof ram, &cpu) ==
          PR_LOADER_0355_AT_CALL_0538);
    expected.dx = 0x0047u;
    expected.ip = 0x0363u;
    expected.flags = 0x7206u;
    CHECK(memcmp(&cpu, &expected, sizeof cpu) == 0);
    CHECK(memcmp(ram, before, sizeof ram) == 0);
    return 0;
}

static int nonzero_rounding(void)
{
    pr_handler_02e6_cpu cpu = frame(0x0021u, 1u);
    CHECK(pr_resident_loader_0355(ram, sizeof ram, &cpu) ==
          PR_LOADER_0355_AT_CALL_0538);
    CHECK(cpu.dx == 4u && cpu.flags == 0x7202u);
    return 0;
}

static int size_wrap(void)
{
    pr_handler_02e6_cpu cpu = frame(0xFFF9u, 0u);
    CHECK(pr_resident_loader_0355(ram, sizeof ram, &cpu) ==
          PR_LOADER_0355_AT_CALL_0538);
    CHECK(cpu.dx == 0u && cpu.flags == 0x7246u);
    return 0;
}

static int final_overflow(void)
{
    pr_handler_02e6_cpu cpu = frame(0x7FF1u, 0x7FFFu);
    CHECK(pr_resident_loader_0355(ram, sizeof ram, &cpu) ==
          PR_LOADER_0355_AT_CALL_0538);
    CHECK(cpu.dx == 0x87FFu && cpu.flags == 0x7A86u);
    return 0;
}

static int final_carry(void)
{
    pr_handler_02e6_cpu cpu = frame(0xFFF0u, 0xF001u);
    CHECK(pr_resident_loader_0355(ram, sizeof ram, &cpu) ==
          PR_LOADER_0355_AT_CALL_0538);
    CHECK(cpu.dx == 0u && cpu.flags == 0x7257u);
    return 0;
}

static int arguments(void)
{
    pr_handler_02e6_cpu cpu = frame(0u, 0x0047u), before = cpu;
    CHECK(pr_resident_loader_0355(NULL, sizeof ram, &cpu) ==
          PR_LOADER_0355_ARGUMENT);
    CHECK(pr_resident_loader_0355(ram, sizeof ram - 1u, &cpu) ==
          PR_LOADER_0355_ARGUMENT);
    CHECK(pr_resident_loader_0355(ram, sizeof ram, NULL) ==
          PR_LOADER_0355_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
    return 0;
}

int main(void)
{
    if (saved_zero() || nonzero_rounding() || size_wrap() ||
        final_overflow() || final_carry() || arguments())
        return 1;
    puts("resident loader +0355 synthetic routes: 6 passed");
    return 0;
}
