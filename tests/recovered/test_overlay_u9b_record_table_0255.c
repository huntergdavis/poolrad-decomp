#include "poolrad/recovered/overlay_u9b_record_table_0255.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do {                                            \
    if (!(condition)) {                                                  \
        fprintf(stderr, "u9B +0255 check failed at line %d: %s\n",      \
                __LINE__, #condition);                                  \
        return 1;                                                        \
    }                                                                    \
} while (0)

static uint8_t ram[PR_U9B_0255_RAM_SIZE];

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U9B_0255_RAM_SIZE - 1u);
}

static uint8_t at(uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static void put(uint16_t segment, uint16_t offset, uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static void put16(uint16_t segment, uint16_t offset, uint16_t value)
{
    put(segment, offset, (uint8_t)value);
    put(segment, (uint16_t)(offset + 1u), (uint8_t)(value >> 8));
}

static uint16_t get16(uint16_t segment, uint16_t offset)
{
    return (uint16_t)(at(segment, offset) |
                      ((uint16_t)at(segment, (uint16_t)(offset + 1u)) << 8));
}

static void put_linear16(uint16_t segment, uint16_t offset,
                         unsigned displacement, uint16_t value)
{
    size_t address = (physical(segment, offset) + displacement) &
                     (PR_U9B_0255_RAM_SIZE - 1u);
    ram[address] = (uint8_t)value;
    ram[(address + 1u) & (PR_U9B_0255_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static pr_u9b_0255_cpu frame(uint16_t ds, uint16_t record_segment,
                            uint16_t record_offset)
{
    pr_u9b_0255_cpu cpu = {
        0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0x3000u, 0x0255u, ds, 0x1111u,
        0x2000u, 0x7202u
    };
    memset(ram, 0, sizeof ram);
    put16(cpu.ss, 0x8000u, 0x7000u); /* return IP */
    put16(cpu.ss, 0x8002u, cpu.cs);  /* return CS */
    put16(cpu.ss, 0x8004u, record_offset);
    put16(cpu.ss, 0x8006u, record_segment);
    return cpu;
}

static int test_empty_and_guard(void)
{
    pr_u9b_0255_cpu cpu = frame(0x1000u, 0x4000u, 0x0100u);
    pr_u9b_0255_cpu original = cpu;
    unsigned outer;

    CHECK(pr_u9b_record_table_0255(NULL, sizeof ram, &cpu) ==
          PR_U9B_0255_ARGUMENT);
    CHECK(memcmp(&cpu, &original, sizeof cpu) == 0);
    CHECK(pr_u9b_record_table_0255(ram, sizeof ram - 1u, &cpu) ==
          PR_U9B_0255_ARGUMENT);
    CHECK(memcmp(&cpu, &original, sizeof cpu) == 0);
    CHECK(pr_u9b_record_table_0255(ram, sizeof ram, NULL) ==
          PR_U9B_0255_ARGUMENT);

    CHECK(pr_u9b_record_table_0255(ram, sizeof ram, &cpu) == PR_U9B_0255_OK);
    for (outer = 0u; outer < 5u; ++outer)
        CHECK(at(0x4000u, (uint16_t)(0x0100u + 0x6Du + outer)) == 0x14u);
    CHECK(cpu.ax == 7u && cpu.bx == 0x3456u && cpu.cx == 0x5678u);
    CHECK(cpu.dx == 0x789Au && cpu.si == 0x9ABCu && cpu.di == 0x0107u);
    CHECK(cpu.es == 0x4000u && cpu.ds == 0x1000u && cpu.ss == 0x2000u);
    CHECK(cpu.bp == 0x7777u && cpu.sp == 0x8008u);
    CHECK(cpu.ip == 0x7000u && cpu.cs == 0x3000u && cpu.flags == 0x7246u);
    CHECK(get16(cpu.ss, 0x7FFEu) == 0x7777u);
    CHECK(at(cpu.ss, 0x7FFDu) == 4u && at(cpu.ss, 0x7FFCu) == 7u);
    return 0;
}

static int test_signed_and_unsigned_gates(void)
{
    static const uint8_t threshold[5] = {3u, 7u, 20u, 30u, 0u};
    static const uint8_t expected[5] = {3u, 7u, 20u, 20u, 0u};
    pr_u9b_0255_cpu cpu = frame(0x1000u, 0x4000u, 0x0100u);
    unsigned outer;

    put(0x4000u, 0x0196u, 1u); /* signed positive inner index zero */
    put(0x4000u, 0x0197u, 0x80u); /* signed negative: no table read */
    for (outer = 0u; outer < 5u; ++outer)
        put(0x1000u, (uint16_t)(0x426Eu + 5u + outer), threshold[outer]);
    CHECK(pr_u9b_record_table_0255(ram, sizeof ram, &cpu) == PR_U9B_0255_OK);
    for (outer = 0u; outer < 5u; ++outer)
        CHECK(at(0x4000u, (uint16_t)(0x016Du + outer)) == expected[outer]);
    CHECK(cpu.ax == 7u && cpu.bx == 4u && cpu.cx == 5u);
    CHECK(cpu.dx == 0u && cpu.si == 1u && cpu.di == 0x0107u);
    CHECK(cpu.flags == 0x7246u);
    return 0;
}

static int test_shared_table_alias(void)
{
    pr_u9b_0255_cpu cpu = frame(0x1000u, 0x1000u, 0x4206u);

    put(0x1000u, 0x429Cu, 1u); /* record +0x96 */
    put(0x1000u, 0x4273u, 40u); /* record +0x6D = DS table[5] */
    CHECK(pr_u9b_record_table_0255(ram, sizeof ram, &cpu) == PR_U9B_0255_OK);
    CHECK(at(0x1000u, 0x4273u) == 0x14u);
    CHECK(at(0x1000u, 0x4274u) == 0x14u);
    CHECK(cpu.ds == cpu.es && cpu.di == 0x420Du);
    return 0;
}

static int test_live_argument_alias(void)
{
    pr_u9b_0255_cpu cpu = frame(0x1000u, 0x2000u, 0x7F97u);
    unsigned outer;

    put(0x2000u, 0x802Du, 1u); /* class at original record +0x96 */
    CHECK(pr_u9b_record_table_0255(ram, sizeof ram, &cpu) == PR_U9B_0255_OK);
    CHECK(get16(cpu.ss, 0x8004u) == 0x7F14u); /* first store aliases arg */
    CHECK(at(cpu.ss, 0x8004u) == 0x14u);
    for (outer = 1u; outer < 5u; ++outer)
        CHECK(at(cpu.ss, (uint16_t)(0x7F81u + outer)) == 0x14u);
    CHECK(cpu.es == cpu.ss && cpu.di == 0x7F1Bu);
    CHECK(cpu.ip == 0x7000u && cpu.cs == 0x3000u);
    return 0;
}

static int test_return_frame_alias(void)
{
    pr_u9b_0255_cpu cpu = frame(0x1000u, 0x2000u, 0x7F93u);

    CHECK(pr_u9b_record_table_0255(ram, sizeof ram, &cpu) == PR_U9B_0255_OK);
    CHECK(get16(cpu.ss, 0x8000u) == 0x1414u);
    CHECK(get16(cpu.ss, 0x8002u) == 0x1414u);
    CHECK(cpu.ip == 0x1414u && cpu.cs == 0x1414u);
    CHECK(cpu.bp == 0x7777u && cpu.sp == 0x8008u);
    return 0;
}

static int test_segment_offset_crossings(void)
{
    pr_u9b_0255_cpu cpu;
    unsigned outer;

    /* BP+6 is FFFF after PUSH BP. LES reads four contiguous physical
     * bytes, including two beyond the segment's FFFF offset. */
    cpu = frame(0x1000u, 0x4000u, 0x0100u);
    cpu.sp = 0xFFFBu;
    put_linear16(cpu.ss, cpu.sp, 0u, 0x7000u);
    put_linear16(cpu.ss, cpu.sp, 2u, cpu.cs);
    put_linear16(cpu.ss, cpu.sp, 4u, 0x0100u);
    put_linear16(cpu.ss, cpu.sp, 6u, 0x4000u);
    CHECK(pr_u9b_record_table_0255(ram, sizeof ram, &cpu) == PR_U9B_0255_OK);
    for (outer = 0u; outer < 5u; ++outer)
        CHECK(at(0x4000u, (uint16_t)(0x016Du + outer)) == 0x14u);
    CHECK(cpu.ip == 0x7000u && cpu.cs == 0x3000u && cpu.sp == 0x0003u);

    /* RETF 4 reads its CS word contiguously after IP at SS:FFFF. */
    cpu = frame(0x1000u, 0x4000u, 0x0100u);
    cpu.sp = 0xFFFFu;
    put_linear16(cpu.ss, cpu.sp, 0u, 0x7000u);
    put_linear16(cpu.ss, cpu.sp, 2u, cpu.cs);
    put_linear16(cpu.ss, cpu.sp, 4u, 0x0100u);
    put_linear16(cpu.ss, cpu.sp, 6u, 0x4000u);
    CHECK(pr_u9b_record_table_0255(ram, sizeof ram, &cpu) == PR_U9B_0255_OK);
    CHECK(cpu.ip == 0x7000u && cpu.cs == 0x3000u && cpu.sp == 0x0007u);
    CHECK(cpu.bp == 0x7777u && cpu.flags == 0x7246u);
    return 0;
}

int main(void)
{
    if (test_empty_and_guard() || test_signed_and_unsigned_gates() ||
        test_shared_table_alias() || test_live_argument_alias() ||
        test_return_frame_alias() || test_segment_offset_crossings())
        return 1;
    puts("u9B +0255 synthetic paths: 6 passed");
    return 0;
}
