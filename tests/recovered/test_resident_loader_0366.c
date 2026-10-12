#include "poolrad/recovered/resident_loader_0366.h"

#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) {                                   \
    fprintf(stderr, "loader +0366 line %d: %s\n", __LINE__, #c); \
    return 1; } } while (0)

static uint8_t ram[PR_HANDLER_02E6_RAM_SIZE];
static uint8_t expected_ram[PR_HANDLER_02E6_RAM_SIZE];

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_HANDLER_02E6_RAM_SIZE - 1u);
}

static void put16(uint8_t *memory, uint16_t seg, uint16_t off,
                  uint16_t value)
{
    memory[at(seg, off)] = (uint8_t)value;
    memory[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(value >> 8);
}

static pr_handler_02e6_cpu frame(uint16_t ax, uint16_t dx)
{
    pr_handler_02e6_cpu cpu = {
        ax, 0x0027u, 0x0004u, dx, 0x44BAu, 0x0000u,
        0x3794u, 0x3778u, 0x12C6u, 0x0366u, 0x14A0u, 0x085Cu,
        0x1D16u, 0x7202u
    };
    memset(ram, 0, sizeof ram);
    put16(ram, cpu.ds, 0x43F6u, 0x2CCDu);
    put16(ram, cpu.ss, cpu.sp, 0x08D0u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp - 2u), 0x0366u);
    return cpu;
}

static int saved_far_call(void)
{
    pr_handler_02e6_cpu cpu = frame(0x0102u, 0x0047u), expected = cpu;
    memcpy(expected_ram, ram, sizeof ram);
    put16(expected_ram, 0x08D0u, 0x0010u, 0x2CCDu);
    put16(expected_ram, cpu.ss, 0x3776u, 0x08D0u);
    CHECK(pr_resident_loader_0366(ram, sizeof ram, &cpu) ==
          PR_LOADER_0366_AT_FAR_CALL);
    expected.ax = 0x2CCDu;
    expected.dx = 0xFF45u;
    expected.es = 0x08D0u;
    expected.sp = 0x3776u;
    expected.ip = 0x03AAu;
    expected.flags = 0x7283u;
    CHECK(memcmp(&cpu, &expected, sizeof cpu) == 0);
    CHECK(memcmp(ram, expected_ram, sizeof ram) == 0);
    return 0;
}

static int unsigned_ja_detour(void)
{
    pr_handler_02e6_cpu cpu = frame(0x0102u, 0x0103u), expected = cpu;
    memcpy(expected_ram, ram, sizeof ram);
    put16(expected_ram, cpu.ss, 0x3776u, 0x0001u);
    CHECK(pr_resident_loader_0366(ram, sizeof ram, &cpu) ==
          PR_LOADER_0366_AT_INTERNAL_CALL);
    expected.dx = 1u;
    expected.sp = 0x3776u;
    expected.ip = 0x0369u;
    CHECK(memcmp(&cpu, &expected, sizeof cpu) == 0);
    CHECK(memcmp(ram, expected_ram, sizeof ram) == 0);
    return 0;
}

static int equal_does_not_jump(void)
{
    pr_handler_02e6_cpu cpu = frame(0x0102u, 0x0102u);
    CHECK(pr_resident_loader_0366(ram, sizeof ram, &cpu) ==
          PR_LOADER_0366_AT_FAR_CALL);
    CHECK(cpu.dx == 0u && cpu.flags == 0x7246u);
    CHECK(cpu.ip == 0x03AAu && cpu.ax == 0x2CCDu);
    return 0;
}

static int overflow_still_jumps_unsigned(void)
{
    pr_handler_02e6_cpu cpu = frame(1u, 0x8000u);
    CHECK(pr_resident_loader_0366(ram, sizeof ram, &cpu) ==
          PR_LOADER_0366_AT_INTERNAL_CALL);
    CHECK(cpu.dx == 0x7FFFu && cpu.flags == 0x7A16u);
    CHECK(cpu.ip == 0x0369u && cpu.ax == 1u);
    return 0;
}

static int stack_offset_wrap(void)
{
    pr_handler_02e6_cpu cpu = frame(0x0102u, 0x0047u);
    size_t next_segment = (((size_t)cpu.ss << 4) + 0x10000u) &
                          (PR_HANDLER_02E6_RAM_SIZE - 1u);
    cpu.sp = 0xFFFFu;
    put16(ram, cpu.ss, cpu.sp, 0x08D0u);
    ram[next_segment] = 0xA5u;
    CHECK(pr_resident_loader_0366(ram, sizeof ram, &cpu) ==
          PR_LOADER_0366_AT_FAR_CALL);
    CHECK(cpu.es == 0x08D0u && cpu.sp == 0xFFFDu);
    CHECK(ram[next_segment] == 0xA5u);
    CHECK(ram[at(cpu.ss, 0xFFFDu)] == 0xD0u);
    CHECK(ram[at(cpu.ss, 0xFFFEu)] == 0x08u);
    return 0;
}

static int arguments(void)
{
    pr_handler_02e6_cpu cpu = frame(0x0102u, 0x0047u), expected = cpu;
    memcpy(expected_ram, ram, sizeof ram);
    CHECK(pr_resident_loader_0366(NULL, sizeof ram, &cpu) ==
          PR_LOADER_0366_ARGUMENT);
    CHECK(pr_resident_loader_0366(ram, sizeof ram - 1u, &cpu) ==
          PR_LOADER_0366_ARGUMENT);
    CHECK(pr_resident_loader_0366(ram, sizeof ram, NULL) ==
          PR_LOADER_0366_ARGUMENT);
    CHECK(memcmp(&cpu, &expected, sizeof cpu) == 0);
    CHECK(memcmp(ram, expected_ram, sizeof ram) == 0);
    return 0;
}

int main(void)
{
    if (saved_far_call() || unsigned_ja_detour() || equal_does_not_jump() ||
        overflow_still_jumps_unsigned() || stack_offset_wrap() || arguments())
        return 1;
    puts("resident loader +0366 synthetic routes: 6 passed");
    return 0;
}
