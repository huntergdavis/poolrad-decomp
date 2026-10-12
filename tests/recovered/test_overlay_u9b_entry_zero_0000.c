#include "poolrad/recovered/overlay_u9b_entry_zero_0000.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do {                                        \
    if (!(condition)) {                                              \
        fprintf(stderr, "u9B +0000 line %d: %s\n", __LINE__, #condition); \
        return 1;                                                    \
    }                                                                \
} while (0)

static uint8_t ram[PR_U9B_0000_RAM_SIZE];

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U9B_0000_RAM_SIZE - 1u);
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
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U9B_0000_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static pr_u9b_0000_cpu frame(void)
{
    pr_u9b_0000_cpu cpu = {
        0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0x3000u, 0x0000u, 0x1000u, 0x1111u,
        0x2000u, 0x7202u
    };
    memset(ram, 0, sizeof ram);
    put16(cpu.ss, 0x8000u, 0x7000u);
    put16(cpu.ss, 0x8002u, 0x3000u);
    put16(cpu.ss, 0x8004u, 0x0100u);
    put16(cpu.ss, 0x8006u, 0x4000u);
    return cpu;
}

static int test_saved_style_skip_and_guards(void)
{
    pr_u9b_0000_cpu cpu = frame();
    pr_u9b_0000_cpu original = cpu;
    unsigned index;
    CHECK(pr_u9b_entry_zero_0000(NULL, sizeof ram, &cpu) ==
          PR_U9B_0000_ARGUMENT);
    CHECK(pr_u9b_entry_zero_0000(ram, sizeof ram - 1u, &cpu) ==
          PR_U9B_0000_ARGUMENT);
    CHECK(pr_u9b_entry_zero_0000(ram, sizeof ram, NULL) ==
          PR_U9B_0000_ARGUMENT);
    CHECK(memcmp(&cpu, &original, sizeof cpu) == 0);

    put(0x4000u, 0x012Du, 0x28u);
    CHECK(pr_u9b_entry_zero_0000(ram, sizeof ram, &cpu) ==
          PR_U9B_0000_OK);
    CHECK(get(0x4000u, 0x012Du) == 0u);
    CHECK(get(0x4000u, 0x0173u) == 0u);
    CHECK(get(0x4000u, 0x01A1u) == 2u);
    for (index = 0u; index < 5u; ++index)
        CHECK(get(0x4000u, (uint16_t)(0x016Du + index)) == 0x14u);
    CHECK(cpu.ip == 0x7000u && cpu.cs == 0x3000u && cpu.sp == 0x8008u);
    CHECK(cpu.bp == 0x7777u);
    return 0;
}

static int test_conditional_increment_leaf(void)
{
    pr_u9b_0000_cpu cpu = frame();
    put(0x4000u, 0x0196u, 2u);
    put(0x4000u, 0x0112u, 0x11u);
    put(0x4000u, 0x01B2u, 1u);
    put(0x4000u, 0x01B3u, 1u);
    put(0x4000u, 0x01B4u, 1u);
    put(0x1000u, 0x40C3u, 1u);
    put(0x1000u, 0x40C4u, 1u);
    put(0x1000u, 0x40C5u, 1u);
    CHECK(pr_u9b_entry_zero_0000(ram, sizeof ram, &cpu) ==
          PR_U9B_0000_OK);
    CHECK(get(0x4000u, 0x01B2u) == 3u);
    CHECK(get(0x4000u, 0x01B3u) == 3u);
    CHECK(get(0x4000u, 0x01B4u) == 2u);
    CHECK(get(cpu.ss, 0x7FFAu) == 0x38u); /* [BP-4] after 56 passes */
    return 0;
}

static int test_conditional_fields_leaf(void)
{
    pr_u9b_0000_cpu cpu = frame();
    put(0x4000u, 0x019Cu, 1u);
    put(0x4000u, 0x012Eu, 1u);
    put(0x1000u, 0x3D44u, 4u);
    put(0x1000u, 0x3CFCu, 7u);
    CHECK(pr_u9b_entry_zero_0000(ram, sizeof ram, &cpu) ==
          PR_U9B_0000_OK);
    CHECK(get(0x4000u, 0x0177u) == 11u);
    CHECK(cpu.ip == 0x7000u && cpu.cs == 0x3000u);
    return 0;
}

static int test_top_return_frame_alias(void)
{
    pr_u9b_0000_cpu cpu = frame();
    put16(cpu.ss, 0x8000u, 0x70AAu);
    put16(cpu.ss, 0x8004u, 0x7FD3u);
    put16(cpu.ss, 0x8006u, cpu.ss);
    CHECK(pr_u9b_entry_zero_0000(ram, sizeof ram, &cpu) ==
          PR_U9B_0000_OK);
    CHECK(cpu.ip == 0x7000u && cpu.cs == 0x3000u);
    CHECK(cpu.sp == 0x8008u && cpu.bp == 0x7777u);
    return 0;
}

static int test_near_return_transfer(void)
{
    pr_u9b_0000_cpu cpu = frame();
    put16(cpu.ss, 0x8004u, 0x7F81u);
    put16(cpu.ss, 0x8006u, cpu.ss);
    CHECK(pr_u9b_entry_zero_0000(ram, sizeof ram, &cpu) ==
          PR_U9B_0000_TRANSFER);
    CHECK(cpu.ip != 0x0194u || cpu.cs != 0x3000u);
    return 0;
}

int main(void)
{
    if (test_saved_style_skip_and_guards() ||
        test_conditional_increment_leaf() ||
        test_conditional_fields_leaf() ||
        test_top_return_frame_alias() ||
        test_near_return_transfer())
        return 1;
    puts("u9B +0000 synthetic routes: 5 passed");
    return 0;
}
