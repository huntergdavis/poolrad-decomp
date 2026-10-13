#include "poolrad/recovered/resident_loader_read_branch_02a1.h"

#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) {                                      \
    fprintf(stderr, "loader branch +02A1 line %d: %s\n", __LINE__, #c); \
    return 1;                                                           \
} } while (0)

static uint8_t ram[PR_HANDLER_02E6_RAM_SIZE];
static uint8_t saved_ram[PR_HANDLER_02E6_RAM_SIZE];

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) &
           (PR_HANDLER_02E6_RAM_SIZE - 1u);
}

static void set_word(uint16_t segment, uint16_t offset, uint16_t value)
{
    ram[physical(segment, offset)] = (uint8_t)value;
    ram[physical(segment, (uint16_t)(offset + 1u))] =
        (uint8_t)(value >> 8);
}

static pr_handler_02e6_cpu request(void)
{
    pr_handler_02e6_cpu cpu = {0};
    cpu.ax = 0x0463u;
    cpu.bx = 5u;
    cpu.cx = 0x0463u;
    cpu.dx = 0u;
    cpu.si = 0x44BAu;
    cpu.di = 0x0824u;
    cpu.bp = 0x3794u;
    cpu.sp = 0x3770u;
    cpu.cs = 0x12C6u;
    cpu.ip = 0x02A1u;
    cpu.ds = 0x2CCDu;
    cpu.es = 0x08D0u;
    cpu.ss = 0x1D16u;
    cpu.flags = 0x7246u;
    return cpu;
}

static int saved_zero_second_count(void)
{
    pr_handler_02e6_cpu cpu = request(), expected = cpu;
    memset(ram, 0x7Du, sizeof ram);
    set_word(cpu.es, 0x0010u, 0x2CCDu);
    set_word(cpu.es, 0x000Au, 0u);
    memcpy(saved_ram, ram, sizeof ram);
    expected.ip = 0x02E1u;
    expected.ax = 0u;
    expected.cx = 0u;
    expected.ds = 0x2D14u;
    CHECK(pr_resident_loader_read_branch_02a1(ram, sizeof ram, &cpu) ==
          PR_LOADER_READ_BRANCH_02A1_AT_EPILOGUE);
    CHECK(memcmp(&cpu, &expected, sizeof cpu) == 0);
    CHECK(memcmp(ram, saved_ram, sizeof ram) == 0);
    return 0;
}

static int error_and_short_read(void)
{
    pr_handler_02e6_cpu cpu = request(), expected;
    memset(ram, 0x31, sizeof ram);
    memcpy(saved_ram, ram, sizeof ram);

    cpu.flags |= 1u;
    expected = cpu;
    expected.ip = 0x02E1u;
    CHECK(pr_resident_loader_read_branch_02a1(ram, sizeof ram, &cpu) ==
          PR_LOADER_READ_BRANCH_02A1_AT_EPILOGUE);
    CHECK(memcmp(&cpu, &expected, sizeof cpu) == 0);

    cpu = request();
    cpu.ax = 2u;
    cpu.cx = 4u;
    cpu.flags = 0x7202u;
    expected = cpu;
    expected.ax = 0x0064u;
    expected.flags = 0x7293u; /* CMP 0002,0004: CF,AF,SF set */
    expected.ip = 0x02E1u;
    CHECK(pr_resident_loader_read_branch_02a1(ram, sizeof ram, &cpu) ==
          PR_LOADER_READ_BRANCH_02A1_AT_EPILOGUE);
    CHECK(memcmp(&cpu, &expected, sizeof cpu) == 0);
    CHECK(memcmp(ram, saved_ram, sizeof ram) == 0);
    return 0;
}

static int second_read_boundary_and_wrap(void)
{
    pr_handler_02e6_cpu cpu = request(), expected;
    memset(ram, 0x48, sizeof ram);
    cpu.ax = cpu.cx = 4u;
    set_word(cpu.es, 0x0010u, 0x3000u);
    set_word(cpu.es, 0x000Au, 2u);
    memcpy(saved_ram, ram, sizeof ram);
    expected = cpu;
    expected.ax = 0x3F01u;
    expected.cx = 2u;
    expected.ds = 0x3001u;
    expected.flags = 0x7246u;
    expected.ip = 0x02C0u;
    CHECK(pr_resident_loader_read_branch_02a1(ram, sizeof ram, &cpu) ==
          PR_LOADER_READ_BRANCH_02A1_AT_SECOND_DOS_READ);
    CHECK(memcmp(&cpu, &expected, sizeof cpu) == 0);
    CHECK(memcmp(ram, saved_ram, sizeof ram) == 0);

    cpu = request();
    cpu.es = 0xFFFFu;
    cpu.ax = cpu.cx = 0xFFFFu;
    set_word(cpu.es, 0x0010u, 0xFFE0u); /* physical 0x00000 */
    set_word(cpu.es, 0x000Au, 0u);
    expected = cpu;
    expected.ax = 0u;
    expected.cx = 0u;
    expected.ds = 0xFFE0u; /* (0xFFFF + 15) wraps before SHR */
    expected.ip = 0x02E1u;
    CHECK(pr_resident_loader_read_branch_02a1(ram, sizeof ram, &cpu) ==
          PR_LOADER_READ_BRANCH_02A1_AT_EPILOGUE);
    CHECK(memcmp(&cpu, &expected, sizeof cpu) == 0);
    return 0;
}

static int invalid_arguments(void)
{
    pr_handler_02e6_cpu cpu = request(), before = cpu;
    memset(ram, 0x93, sizeof ram);
    memcpy(saved_ram, ram, sizeof ram);
    CHECK(pr_resident_loader_read_branch_02a1(NULL, sizeof ram, &cpu) ==
          PR_LOADER_READ_BRANCH_02A1_ARGUMENT);
    CHECK(pr_resident_loader_read_branch_02a1(ram, sizeof ram - 1u, &cpu) ==
          PR_LOADER_READ_BRANCH_02A1_ARGUMENT);
    CHECK(pr_resident_loader_read_branch_02a1(ram, sizeof ram, NULL) ==
          PR_LOADER_READ_BRANCH_02A1_ARGUMENT);
    cpu.cs = 0x12C7u;
    CHECK(pr_resident_loader_read_branch_02a1(ram, sizeof ram, &cpu) ==
          PR_LOADER_READ_BRANCH_02A1_ARGUMENT);
    cpu = before;
    cpu.ip = 0x02A3u;
    CHECK(pr_resident_loader_read_branch_02a1(ram, sizeof ram, &cpu) ==
          PR_LOADER_READ_BRANCH_02A1_ARGUMENT);
    CHECK(memcmp(ram, saved_ram, sizeof ram) == 0);
    return 0;
}

int main(void)
{
    if (saved_zero_second_count() || error_and_short_read() ||
        second_read_boundary_and_wrap() || invalid_arguments())
        return 1;
    puts("resident loader +02A1 branch synthetic paths: passed");
    return 0;
}
