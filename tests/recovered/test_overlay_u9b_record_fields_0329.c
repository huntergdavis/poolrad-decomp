#include "poolrad/recovered/overlay_u9b_record_fields_0329.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do {                                           \
    if (!(condition)) {                                                 \
        fprintf(stderr, "u9B +0329 line %d: %s\n", __LINE__, #condition); \
        return 1;                                                       \
    }                                                                   \
} while (0)

static uint8_t ram[PR_U9B_0329_RAM_SIZE];

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U9B_0329_RAM_SIZE - 1u);
}

static void put(uint16_t segment, uint16_t offset, uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static uint8_t get(uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static void put16(uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t address = physical(segment, offset);
    ram[address] = (uint8_t)value;
    ram[(address + 1u) & (PR_U9B_0329_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static pr_u9b_0329_cpu frame(uint16_t record_segment, uint16_t record_offset)
{
    pr_u9b_0329_cpu cpu = {
        0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0x3000u, 0x0329u, 0x1000u, 0x1111u,
        0x2000u, 0x7202u
    };
    memset(ram, 0, sizeof ram);
    put16(cpu.ss, cpu.sp, 0x7000u);
    put16(cpu.ss, (uint16_t)(cpu.sp + 2u), cpu.cs);
    put16(cpu.ss, (uint16_t)(cpu.sp + 4u), record_offset);
    put16(cpu.ss, (uint16_t)(cpu.sp + 6u), record_segment);
    return cpu;
}

static int test_empty_and_guard(void)
{
    pr_u9b_0329_cpu cpu = frame(0x4000u, 0x0100u);
    pr_u9b_0329_cpu original = cpu;
    unsigned index;

    CHECK(pr_u9b_record_fields_0329(NULL, sizeof ram, &cpu) ==
          PR_U9B_0329_ARGUMENT);
    CHECK(pr_u9b_record_fields_0329(ram, sizeof ram - 1u, &cpu) ==
          PR_U9B_0329_ARGUMENT);
    CHECK(pr_u9b_record_fields_0329(ram, sizeof ram, NULL) ==
          PR_U9B_0329_ARGUMENT);
    CHECK(memcmp(&cpu, &original, sizeof cpu) == 0);

    for (index = 1u; index <= 8u; ++index)
        put(0x4000u, (uint16_t)(0x0176u + index), 0xAAu);
    CHECK(pr_u9b_record_fields_0329(ram, sizeof ram, &cpu) ==
          PR_U9B_0329_OK);
    for (index = 1u; index <= 8u; ++index)
        CHECK(get(0x4000u, (uint16_t)(0x0176u + index)) == 0u);
    CHECK(cpu.ip == 0x7000u && cpu.cs == 0x3000u && cpu.sp == 0x8008u);
    CHECK(cpu.bp == 0x7777u && cpu.ds == 0x1000u && cpu.ss == 0x2000u);
    CHECK(cpu.flags == 0x7246u);
    CHECK(get(cpu.ss, 0x7FFDu) == 8u);
    return 0;
}

static int test_signed_gate_and_bonus(void)
{
    pr_u9b_0329_cpu cpu = frame(0x4000u, 0x0100u);
    unsigned index;
    put(0x4000u, 0x012Eu, 1u);
    put(0x4000u, 0x019Cu, 2u);
    put(0x4000u, 0x0113u, 3u);
    for (index = 1u; index <= 8u; ++index) {
        put(0x1000u, (uint16_t)(0x3D3Bu + 8u + index), 4u);
        put(0x1000u, (uint16_t)(0x3CF3u + 16u + index), 7u);
        if (index < 6u)
            put(0x1000u, (uint16_t)(0x3D4Eu + 15u + index), 2u);
    }
    /* Negative signed source: abs(10)>7 clears index 1. */
    put(0x1000u, 0x3D44u, 0xF6u);
    /* abs(3)<=7 enters the sum path, then adds the signed bonus. */
    put(0x1000u, 0x3D45u, 0xFDu);
    CHECK(pr_u9b_record_fields_0329(ram, sizeof ram, &cpu) ==
          PR_U9B_0329_OK);
    CHECK(get(0x4000u, 0x0177u) == 0u);
    CHECK(get(0x4000u, 0x0178u) == 6u);
    for (index = 3u; index < 6u; ++index)
        CHECK(get(0x4000u, (uint16_t)(0x0176u + index)) == 13u);
    for (index = 6u; index <= 8u; ++index)
        CHECK(get(0x4000u, (uint16_t)(0x0176u + index)) == 11u);
    return 0;
}

static int test_return_frame_alias(void)
{
    pr_u9b_0329_cpu cpu = frame(0x2000u, 0x7F89u);
    CHECK(pr_u9b_record_fields_0329(ram, sizeof ram, &cpu) ==
          PR_U9B_0329_OK);
    CHECK(cpu.ip == 0u && cpu.cs == 0u);
    CHECK(cpu.bp == 0x7777u && cpu.sp == 0x8008u);
    return 0;
}

int main(void)
{
    if (test_empty_and_guard() || test_signed_gate_and_bonus() ||
        test_return_frame_alias())
        return 1;
    puts("u9B +0329 synthetic paths: 3 passed");
    return 0;
}
