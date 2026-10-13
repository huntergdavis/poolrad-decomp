#include "poolrad/recovered/resident_loader_read_gate_0291.h"

#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) {                                         \
    fprintf(stderr, "loader read +0291 line %d: %s\n", __LINE__, #c);   \
    return 1;                                                              \
} } while (0)

static uint8_t ram[PR_HANDLER_02E6_RAM_SIZE];
static uint8_t saved_ram[PR_HANDLER_02E6_RAM_SIZE];

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_HANDLER_02E6_RAM_SIZE - 1u);
}

static void put16(uint16_t seg, uint16_t off, uint16_t value)
{
    ram[at(seg, off)] = (uint8_t)value;
    ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(value >> 8);
}

static pr_handler_02e6_cpu entry(void)
{
    pr_handler_02e6_cpu cpu = {
        0xAA87u, 0x0005u, 0x0002u, 0x0002u, 0x44BAu, 0x0824u,
        0x3794u, 0x3770u, 0x12C6u, 0x0291u, 0x14A0u, 0x08D0u,
        0x1D16u, 0x7202u
    };
    memset(ram, 0x31, sizeof ram);
    put16(cpu.es, 0x0008u, 0x0463u);
    put16(cpu.es, 0x0010u, 0x2CCDu);
    return cpu;
}

static int saved_read_setup(void)
{
    pr_handler_02e6_cpu cpu = entry();
    pr_handler_02e6_cpu expected = cpu;
    memcpy(saved_ram, ram, sizeof ram);
    expected.ds = 0x2CCDu;
    expected.dx = 0u;
    expected.cx = 0x0463u;
    expected.ax = 0x3F87u;
    expected.ip = 0x029Fu;
    expected.flags = 0x7246u;
    CHECK(pr_resident_loader_read_gate_0291(ram, sizeof ram, &cpu) ==
          PR_LOADER_READ_GATE_0291_AT_DOS_READ);
    CHECK(memcmp(&cpu, &expected, sizeof cpu) == 0);
    CHECK(memcmp(ram, saved_ram, sizeof ram) == 0);
    return 0;
}

static int alternate_table_and_low_al(void)
{
    pr_handler_02e6_cpu cpu = entry();
    cpu.ax = 0xAB05u;
    cpu.flags = 0x7AD7u;
    put16(cpu.es, 0x0008u, 0xFFFFu);
    put16(cpu.es, 0x0010u, 0x0000u);
    CHECK(pr_resident_loader_read_gate_0291(ram, sizeof ram, &cpu) ==
          PR_LOADER_READ_GATE_0291_AT_DOS_READ);
    CHECK(cpu.ax == 0x3F05u && cpu.cx == 0xFFFFu);
    CHECK(cpu.ds == 0u && cpu.dx == 0u);
    CHECK(cpu.flags == 0x7246u);
    return 0;
}

static int segment_bus_wrap(void)
{
    pr_handler_02e6_cpu cpu = entry();
    cpu.es = 0xFFFFu;
    put16(cpu.es, 0x0008u, 0x1234u);
    put16(cpu.es, 0x0010u, 0x5678u);
    CHECK(pr_resident_loader_read_gate_0291(ram, sizeof ram, &cpu) ==
          PR_LOADER_READ_GATE_0291_AT_DOS_READ);
    CHECK(cpu.cx == 0x1234u && cpu.ds == 0x5678u);
    CHECK(cpu.ax == 0x3F87u && cpu.ip == 0x029Fu);
    return 0;
}

static int invalid_arguments(void)
{
    pr_handler_02e6_cpu cpu = entry();
    pr_handler_02e6_cpu expected = cpu;
    memcpy(saved_ram, ram, sizeof ram);
    CHECK(pr_resident_loader_read_gate_0291(NULL, sizeof ram, &cpu) ==
          PR_LOADER_READ_GATE_0291_ARGUMENT);
    CHECK(pr_resident_loader_read_gate_0291(ram, sizeof ram - 1u, &cpu) ==
          PR_LOADER_READ_GATE_0291_ARGUMENT);
    CHECK(pr_resident_loader_read_gate_0291(ram, sizeof ram, NULL) ==
          PR_LOADER_READ_GATE_0291_ARGUMENT);
    cpu.ip = 0x029Fu;
    CHECK(pr_resident_loader_read_gate_0291(ram, sizeof ram, &cpu) ==
          PR_LOADER_READ_GATE_0291_ARGUMENT);
    cpu = expected;
    CHECK(memcmp(&cpu, &expected, sizeof cpu) == 0);
    CHECK(memcmp(ram, saved_ram, sizeof ram) == 0);
    return 0;
}

int main(void)
{
    if (saved_read_setup() || alternate_table_and_low_al() ||
        segment_bus_wrap() || invalid_arguments())
        return 1;
    puts("resident loader +0291 synthetic read setup: 4 passed");
    return 0;
}
