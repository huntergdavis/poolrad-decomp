#include "poolrad/recovered/resident_handler_prefix_02e6.h"

#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) {                                      \
    fprintf(stderr, "handler +02E6 line %d: %s\n", __LINE__, #c);   \
    return 1; } } while (0)

static uint8_t ram[PR_HANDLER_02E6_RAM_SIZE];

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_HANDLER_02E6_RAM_SIZE - 1u);
}

static void put16(uint16_t seg, uint16_t off, uint16_t value)
{
    size_t address = at(seg, off);
    ram[address] = (uint8_t)value;
    ram[(address + 1u) & (PR_HANDLER_02E6_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(uint16_t seg, uint16_t off)
{
    size_t address = at(seg, off);
    return (uint16_t)(ram[address] |
                      ((uint16_t)ram[(address + 1u) &
                                      (PR_HANDLER_02E6_RAM_SIZE - 1u)] << 8));
}

static pr_handler_02e6_cpu frame(uint16_t saved_ip)
{
    pr_handler_02e6_cpu cpu = {
        0x0000u, 0x001Fu, 0x0007u, 0x449Au, 0x44BAu, 0x0000u,
        0x37AEu, 0x3790u, 0x12C6u, 0x02E6u, 0x14A0u, 0x3A16u,
        0x1D16u, 0x7002u
    };
    memset(ram, 0, sizeof ram);
    put16(cpu.cs, 0x02F2u, 0x14A0u);
    put16(cpu.ss, cpu.sp, saved_ip);
    put16(cpu.ss, (uint16_t)(cpu.sp + 2u), 0x08D0u);
    put16(cpu.ss, (uint16_t)(cpu.sp + 4u), 0x7202u);
    put16(0x08D0u, saved_ip, 0x9966u);
    return cpu;
}

static int saved_branch(void)
{
    pr_handler_02e6_cpu cpu = frame(0x0027u);
    CHECK(pr_resident_handler_prefix_02e6(ram, sizeof ram, &cpu) ==
          PR_HANDLER_02E6_AT_LOADER);
    CHECK(cpu.cs == 0x12C6u && cpu.ip == 0x0314u);
    CHECK(cpu.ax == 0x37AEu && cpu.bx == 0x0027u);
    CHECK(cpu.cx == 7u && cpu.dx == 0x449Au);
    CHECK(cpu.si == 0x44BAu && cpu.di == 0u);
    CHECK(cpu.ds == 0x14A0u && cpu.es == 0x08D0u && cpu.ss == 0x1D16u);
    CHECK(cpu.sp == 0x377Cu && cpu.bp == 0x3794u && cpu.flags == 0x7212u);
    CHECK(get16(cpu.ss, 0x377Cu) == 0x9966u);
    CHECK(get16(cpu.ss, 0x378Eu) == 0x7202u);
    CHECK(get16(cpu.ss, 0x3790u) == 0x0025u);
    CHECK(get16(cpu.ss, 0x3794u) == 0x37AEu);
    CHECK(get16(cpu.ss, 0x3780u) == 0x14A0u);
    CHECK(get16(cpu.ss, 0x377Eu) == 0x3A16u);
    return 0;
}

static int equal_branch(void)
{
    pr_handler_02e6_cpu cpu = frame(0x0002u);
    put16(cpu.cs, 0x02F2u, 0x4321u);
    CHECK(pr_resident_handler_prefix_02e6(ram, sizeof ram, &cpu) ==
          PR_HANDLER_02E6_AT_LOADER);
    CHECK(cpu.ip == 0x0303u && cpu.sp == 0x377Cu && cpu.bp == 0x378Eu);
    CHECK(cpu.ax == 0x4321u && cpu.ds == 0x4321u);
    CHECK(cpu.bx == 0x0002u && cpu.es == 0x08D0u);
    CHECK(cpu.flags == 0x7246u);
    CHECK(get16(cpu.ss, 0x3790u) == 0u);
    CHECK(get16(cpu.ss, 0x3794u) == 0x7202u);
    CHECK(get16(cpu.ss, 0x378Eu) == 0x37AEu);
    return 0;
}

static int segment_wrap(void)
{
    pr_handler_02e6_cpu cpu = frame(0x0027u);
    cpu.ss = 0xFFFFu;
    cpu.sp = 0u;
    cpu.bp = 0xA55Au;
    put16(cpu.ss, 0u, 2u);
    put16(cpu.ss, 2u, 0x08D0u);
    put16(cpu.ss, 4u, 0x7002u);
    CHECK(pr_resident_handler_prefix_02e6(ram, sizeof ram, &cpu) ==
          PR_HANDLER_02E6_AT_LOADER);
    CHECK(cpu.ip == 0x0303u && cpu.sp == 0xFFECu);
    CHECK(get16(cpu.ss, 0xFFFEu) == 0xA55Au);
    CHECK(get16(cpu.ss, 0u) == 0u);
    return 0;
}

static int arguments(void)
{
    pr_handler_02e6_cpu cpu = frame(0x0027u), before = cpu;
    CHECK(pr_resident_handler_prefix_02e6(NULL, sizeof ram, &cpu) ==
          PR_HANDLER_02E6_ARGUMENT);
    CHECK(pr_resident_handler_prefix_02e6(ram, sizeof ram - 1u, &cpu) ==
          PR_HANDLER_02E6_ARGUMENT);
    CHECK(pr_resident_handler_prefix_02e6(ram, sizeof ram, NULL) ==
          PR_HANDLER_02E6_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
    CHECK(get16(cpu.ss, cpu.sp) == 0x0027u);
    return 0;
}

int main(void)
{
    if (saved_branch() || equal_branch() || segment_wrap() || arguments())
        return 1;
    puts("resident handler +02E6 prefix synthetic routes: 4 passed");
    return 0;
}
