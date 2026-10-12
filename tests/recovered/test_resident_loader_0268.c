#include "poolrad/recovered/resident_loader_0268.h"

#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) {                                   \
    fprintf(stderr, "loader +0268 line %d: %s\n", __LINE__, #c); \
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

static pr_handler_02e6_cpu frame(uint16_t low, uint16_t high,
                                  uint16_t base_low, uint16_t base_high)
{
    pr_handler_02e6_cpu cpu = {
        0x2CCDu, 0x0027u, 0x0004u, 0xFF45u, 0x44BAu, 0x0000u,
        0x3794u, 0x3772u, 0x12C6u, 0x0268u, 0x14A0u, 0x08D0u,
        0x1D16u, 0x7283u
    };
    memset(ram, 0, sizeof ram);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u), 0x08D0u);
    put16(ram, cpu.ss, cpu.sp, 0x03AEu);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0x12C6u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp - 2u), 0x0008u);
    put16(ram, cpu.ds, 0x43FCu, 0x0005u);
    put16(ram, cpu.ds, 0x4422u, 0x0814u);
    put16(ram, cpu.ds, 0x43E8u, base_low);
    put16(ram, cpu.ds, 0x43EAu, base_high);
    put16(ram, 0x08D0u, 0x0004u, low);
    put16(ram, 0x08D0u, 0x0006u, high);
    return cpu;
}

static int saved_seek_setup(void)
{
    pr_handler_02e6_cpu cpu = frame(0xAA87u, 0x0002u, 0u, 0u);
    pr_handler_02e6_cpu expected = cpu;
    memcpy(expected_ram, ram, sizeof ram);
    put16(expected_ram, cpu.ss, 0x3770u, cpu.ds);
    CHECK(pr_resident_loader_0268(ram, sizeof ram, &cpu) ==
          PR_LOADER_0268_AT_DOS_SEEK);
    expected.ax = 0x4200u;
    expected.bx = 0x0005u;
    expected.cx = 0x0002u;
    expected.dx = 0xAA87u;
    expected.di = 0x0824u;
    expected.sp = 0x3770u;
    expected.ip = 0x028Fu;
    expected.flags = 0x7202u;
    CHECK(memcmp(&cpu, &expected, sizeof cpu) == 0);
    CHECK(memcmp(ram, expected_ram, sizeof ram) == 0);
    return 0;
}

static int low_carry_into_high(void)
{
    pr_handler_02e6_cpu cpu = frame(0xFFFFu, 0u, 1u, 0u);
    CHECK(pr_resident_loader_0268(ram, sizeof ram, &cpu) ==
          PR_LOADER_0268_AT_DOS_SEEK);
    CHECK(cpu.dx == 0u && cpu.cx == 1u && cpu.flags == 0x7202u);
    return 0;
}

static int high_overflow_from_carry(void)
{
    pr_handler_02e6_cpu cpu = frame(0xFFFFu, 0x7FFFu, 1u, 0u);
    CHECK(pr_resident_loader_0268(ram, sizeof ram, &cpu) ==
          PR_LOADER_0268_AT_DOS_SEEK);
    CHECK(cpu.dx == 0u && cpu.cx == 0x8000u && cpu.flags == 0x7A96u);
    return 0;
}

static int high_carry_and_zero(void)
{
    pr_handler_02e6_cpu cpu = frame(0xFFFFu, 0xFFFFu, 1u, 0u);
    CHECK(pr_resident_loader_0268(ram, sizeof ram, &cpu) ==
          PR_LOADER_0268_AT_DOS_SEEK);
    CHECK(cpu.dx == 0u && cpu.cx == 0u && cpu.flags == 0x7257u);
    return 0;
}

static int di_wrap_and_stack_offset_wrap(void)
{
    pr_handler_02e6_cpu cpu = frame(0xAA87u, 0x0002u, 0u, 0u);
    cpu.sp = 0xFFFEu;
    put16(ram, cpu.ss, 0x0002u, 0x08D0u);
    put16(ram, cpu.ds, 0x4422u, 0xFFF5u);
    CHECK(pr_resident_loader_0268(ram, sizeof ram, &cpu) ==
          PR_LOADER_0268_AT_DOS_SEEK);
    CHECK(cpu.di == 0x0005u && cpu.sp == 0xFFFCu);
    CHECK(ram[at(cpu.ss, 0xFFFCu)] == 0xA0u);
    CHECK(ram[at(cpu.ss, 0xFFFDu)] == 0x14u);
    return 0;
}

static int arguments(void)
{
    pr_handler_02e6_cpu cpu = frame(0xAA87u, 0x0002u, 0u, 0u);
    pr_handler_02e6_cpu expected = cpu;
    memcpy(expected_ram, ram, sizeof ram);
    CHECK(pr_resident_loader_0268(NULL, sizeof ram, &cpu) ==
          PR_LOADER_0268_ARGUMENT);
    CHECK(pr_resident_loader_0268(ram, sizeof ram - 1u, &cpu) ==
          PR_LOADER_0268_ARGUMENT);
    CHECK(pr_resident_loader_0268(ram, sizeof ram, NULL) ==
          PR_LOADER_0268_ARGUMENT);
    CHECK(memcmp(&cpu, &expected, sizeof cpu) == 0);
    CHECK(memcmp(ram, expected_ram, sizeof ram) == 0);
    return 0;
}

int main(void)
{
    if (saved_seek_setup() || low_carry_into_high() ||
        high_overflow_from_carry() || high_carry_and_zero() ||
        di_wrap_and_stack_offset_wrap() || arguments())
        return 1;
    puts("resident loader +0268 synthetic routes: 6 passed");
    return 0;
}
