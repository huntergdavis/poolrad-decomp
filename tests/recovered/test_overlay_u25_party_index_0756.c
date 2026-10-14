#include "poolrad/recovered/overlay_u25_party_index_0756.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do {                                            \
    if (!(condition)) {                                                  \
        fprintf(stderr, "u25 +0756 check failed at line %d: %s\n",     \
                __LINE__, #condition);                                  \
        return 1;                                                        \
    }                                                                    \
} while (0)

static uint8_t ram[PR_U25_0756_RAM_SIZE];

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U25_0756_RAM_SIZE - 1u);
}

static uint8_t at(uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static void put16(uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t address = physical(segment, offset);
    ram[address] = (uint8_t)value;
    ram[(address + 1u) & (PR_U25_0756_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(uint16_t segment, uint16_t offset)
{
    size_t address = physical(segment, offset);
    return (uint16_t)(ram[address] |
                      ((uint16_t)ram[(address + 1u) &
                                      (PR_U25_0756_RAM_SIZE - 1u)] << 8));
}

static pr_u25_0756_cpu frame(uint16_t head_segment, uint16_t head_offset,
                             uint16_t arg_segment, uint16_t arg_offset)
{
    pr_u25_0756_cpu cpu = {
        0xABCDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0x3000u, 0x0756u, 0x1000u, 0x1111u,
        0x2000u, 0x7202u
    };
    memset(ram, 0, sizeof ram);
    put16(cpu.ss, cpu.sp, 0x7000u);
    put16(cpu.ss, (uint16_t)(cpu.sp + 2u), cpu.cs);
    put16(cpu.ss, (uint16_t)(cpu.sp + 4u), arg_offset);
    put16(cpu.ss, (uint16_t)(cpu.sp + 6u), arg_segment);
    put16(cpu.ds, 0x5D96u, head_offset);
    put16(cpu.ds, 0x5D98u, head_segment);
    return cpu;
}

static int check_return(const pr_u25_0756_cpu *cpu)
{
    CHECK(cpu->bx == 0x3456u && cpu->cx == 0x5678u && cpu->si == 0x9ABCu);
    CHECK(cpu->bp == 0x7777u && cpu->sp == 0x8008u);
    CHECK(cpu->ip == 0x7000u && cpu->cs == 0x3000u);
    CHECK(cpu->ds == 0x1000u && cpu->ss == 0x2000u);
    CHECK(get16(cpu->ss, 0x7FFEu) == 0x7777u);
    return 0;
}

static int test_empty_and_arguments(void)
{
    pr_u25_0756_cpu cpu = frame(0u, 0u, 0x4000u, 0x0100u);
    pr_u25_0756_cpu before = cpu;
    CHECK(pr_u25_party_index_0756(NULL, sizeof ram, &cpu) ==
          PR_U25_0756_ARGUMENT);
    CHECK(pr_u25_party_index_0756(ram, sizeof ram - 1u, &cpu) ==
          PR_U25_0756_ARGUMENT);
    CHECK(pr_u25_party_index_0756(ram, sizeof ram, NULL) ==
          PR_U25_0756_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
    CHECK(pr_u25_party_index_0756(ram, sizeof ram, &cpu) == PR_U25_0756_OK);
    CHECK(check_return(&cpu) == 0);
    CHECK(cpu.ax == 0u && cpu.dx == 0u && cpu.di == 0xBCDEu);
    CHECK((cpu.flags & 0x08C5u) == 0x0044u);
    CHECK(cpu.es == 0x1111u);
    CHECK(at(cpu.ss, 0x7FF7u) == 0u && at(cpu.ss, 0x7FFCu) == 0u);
    CHECK(at(cpu.ss, 0x7FFDu) == 0u);
    return 0;
}

static int test_found_and_missing(void)
{
    pr_u25_0756_cpu cpu = frame(0x4000u, 0x0100u, 0x4000u, 0x0100u);
    put16(0x4000u, 0x0204u, 0x0200u);
    put16(0x4000u, 0x0206u, 0x5000u);
    CHECK(pr_u25_party_index_0756(ram, sizeof ram, &cpu) == PR_U25_0756_OK);
    CHECK(check_return(&cpu) == 0);
    CHECK(cpu.ax == 0x4100u && cpu.dx == 0x4000u);
    CHECK((cpu.flags & 0x08C5u) == 0u);
    CHECK(cpu.di == 0xBCDEu && cpu.es == 0x1111u);
    CHECK(at(cpu.ss, 0x7FF7u) == 1u && at(cpu.ss, 0x7FFCu) == 0u);

    cpu = frame(0x4000u, 0x0100u, 0x5000u, 0x0200u);
    put16(0x4000u, 0x0204u, 0x0200u);
    put16(0x4000u, 0x0206u, 0x5000u);
    CHECK(pr_u25_party_index_0756(ram, sizeof ram, &cpu) == PR_U25_0756_OK);
    CHECK(check_return(&cpu) == 0);
    CHECK(cpu.ax == 0x5201u && cpu.dx == 0x5000u);
    CHECK((cpu.flags & 0x08C5u) == 0u);
    CHECK(cpu.di == 0x0100u && cpu.es == 0x4000u);
    CHECK(at(cpu.ss, 0x7FFCu) == 1u);

    cpu = frame(0x4000u, 0x0100u, 0x6000u, 0x0300u);
    put16(0x4000u, 0x0204u, 0x0200u);
    put16(0x4000u, 0x0206u, 0x5000u);
    CHECK(pr_u25_party_index_0756(ram, sizeof ram, &cpu) == PR_U25_0756_OK);
    CHECK(check_return(&cpu) == 0);
    CHECK(cpu.ax == 2u && cpu.dx == 0u);
    CHECK((cpu.flags & 0x08C5u) == 0x0044u);
    CHECK(cpu.di == 0x0200u && cpu.es == 0x5000u);
    CHECK(at(cpu.ss, 0x7FFCu) == 2u && at(cpu.ss, 0x7FF7u) == 0u);
    return 0;
}

static int test_byte_wrap(void)
{
    pr_u25_0756_cpu cpu = frame(0x4000u, 0x0100u, 0x7000u, 0x0300u);
    unsigned i;
    for (i = 0u; i < 257u; ++i) {
        uint16_t segment = (uint16_t)(0x4000u + i * 0x20u);
        uint16_t next = (uint16_t)(segment + 0x20u);
        put16(segment, 0x0204u, i == 256u ? 0u : 0x0100u);
        put16(segment, 0x0206u, i == 256u ? 0u : next);
    }
    CHECK(pr_u25_party_index_0756(ram, sizeof ram, &cpu) == PR_U25_0756_OK);
    CHECK(check_return(&cpu) == 0);
    CHECK(cpu.ax == 1u && at(cpu.ss, 0x7FFCu) == 1u);
    CHECK((cpu.flags & 0x08C5u) == 0x0044u);
    CHECK(cpu.di == 0x0100u && cpu.es == 0x6000u);
    return 0;
}

int main(void)
{
    if (test_empty_and_arguments() || test_found_and_missing() ||
        test_byte_wrap())
        return 1;
    puts("u25 +0756 party-index tests passed");
    return 0;
}
