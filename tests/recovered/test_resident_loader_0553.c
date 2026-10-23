#include "poolrad/recovered/resident_loader_0553.h"

#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) {                                   \
    fprintf(stderr, "loader +0553 line %d: %s\n", __LINE__, #c); \
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

static pr_handler_02e6_cpu frame(uint16_t input)
{
    pr_handler_02e6_cpu cpu = {
        0x37AEu, 0x0027u, 0x0007u, 0x449Au, 0x44BAu, 0x0000u,
        0x3794u, 0x3776u, 0x12C6u, 0x0553u, 0x14A0u, 0x08D0u,
        0x1D16u, 0x7202u
    };
    memset(ram, 0, sizeof ram);
    put16(cpu.es, 0x0008u, input);
    put16(cpu.ss, cpu.sp, 0x0355u);
    return cpu;
}

static int saved_input(void)
{
    pr_handler_02e6_cpu cpu = frame(0x0463u), expected = cpu;
    uint8_t before[sizeof ram];
    memcpy(before, ram, sizeof ram);
    CHECK(pr_resident_loader_0553(ram, sizeof ram, &cpu) ==
          PR_LOADER_0553_RETURNED);
    expected.ax = 0x0047u;
    expected.cx = 0x0004u;
    expected.ip = 0x0355u;
    expected.sp = 0x3778u;
    expected.flags = 0x7216u;
    CHECK(memcmp(&cpu, &expected, sizeof cpu) == 0);
    CHECK(memcmp(ram, before, sizeof ram) == 0);
    return 0;
}

static int zero_size(void)
{
    pr_handler_02e6_cpu cpu = frame(0u);
    CHECK(pr_resident_loader_0553(ram, sizeof ram, &cpu) ==
          PR_LOADER_0553_RETURNED);
    CHECK(cpu.ax == 0u && cpu.cx == 4u);
    CHECK(cpu.flags == 0x7257u); /* SHR CF/PF/ZF; pinned SHR sets AF (count 4) and clears OF. */
    return 0;
}

static int wrapping_size(void)
{
    pr_handler_02e6_cpu cpu = frame(0xFFF9u);
    CHECK(pr_resident_loader_0553(ram, sizeof ram, &cpu) ==
          PR_LOADER_0553_RETURNED);
    CHECK(cpu.ax == 0u && cpu.flags == 0x7257u);
    return 0;
}

static int stack_word_crossing(void)
{
    pr_handler_02e6_cpu cpu = frame(0x0463u);
    size_t next_segment = (((size_t)cpu.ss << 4) + 0x10000u) &
                          (PR_HANDLER_02E6_RAM_SIZE - 1u);
    cpu.sp = 0xFFFFu;
    put16(cpu.ss, cpu.sp, 0x1234u);
    ram[next_segment] = 0xA5u;
    CHECK(pr_resident_loader_0553(ram, sizeof ram, &cpu) ==
          PR_LOADER_0553_RETURNED);
    CHECK(cpu.ip == 0x1234u && cpu.sp == 0x0001u);
    CHECK(ram[next_segment] == 0xA5u);
    return 0;
}

static int arguments(void)
{
    pr_handler_02e6_cpu cpu = frame(0x0463u), before = cpu;
    CHECK(pr_resident_loader_0553(NULL, sizeof ram, &cpu) ==
          PR_LOADER_0553_ARGUMENT);
    CHECK(pr_resident_loader_0553(ram, sizeof ram - 1u, &cpu) ==
          PR_LOADER_0553_ARGUMENT);
    CHECK(pr_resident_loader_0553(ram, sizeof ram, NULL) ==
          PR_LOADER_0553_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
    CHECK(ram[at(cpu.es, 0x0008u)] == 0x63u);
    return 0;
}

int main(void)
{
    if (saved_input() || zero_size() || wrapping_size() ||
        stack_word_crossing() || arguments())
        return 1;
    puts("resident loader +0553 synthetic routes: 5 passed");
    return 0;
}
