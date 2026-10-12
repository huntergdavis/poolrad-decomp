#include "poolrad/recovered/overlay_u5e_party_value_2879.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do {                                          \
    if (!(condition)) {                                                \
        fprintf(stderr, "u5E +2879 line %d: %s\n", __LINE__, #condition); \
        return 1;                                                      \
    }                                                                  \
} while (0)

static uint8_t ram[PR_U5E_2879_RAM_SIZE];

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U5E_2879_RAM_SIZE - 1u);
}

static void put8(uint16_t segment, uint16_t offset, uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static uint8_t get8(uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static void put16(uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U5E_2879_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static pr_u5e_2879_cpu frame(void)
{
    pr_u5e_2879_cpu cpu = {
        0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0x3000u, 0x2879u, 0x1000u, 0x1111u,
        0x2000u, 0x7202u
    };
    memset(ram, 0, sizeof ram);
    put16(cpu.ss, cpu.sp, 0x7000u);
    put16(cpu.ss, (uint16_t)(cpu.sp + 2u), cpu.cs);
    return cpu;
}

static void head(uint16_t offset, uint16_t segment)
{
    put16(0x1000u, 0x5D96u, offset);
    put16(0x1000u, 0x5D98u, segment);
}

static void node(uint16_t offset, uint16_t next_offset, uint16_t next_segment,
                 uint8_t eligible, uint8_t gate, uint8_t numerator,
                 uint8_t denominator)
{
    put16(0x4000u, (uint16_t)(offset + 0x104u), next_offset);
    put16(0x4000u, (uint16_t)(offset + 0x106u), next_segment);
    put8(0x4000u, (uint16_t)(offset + 0x10Eu), eligible);
    put8(0x4000u, (uint16_t)(offset + 0x10Du), gate);
    put8(0x4000u, (uint16_t)(offset + 0x11Bu), numerator);
    put8(0x4000u, (uint16_t)(offset + 0x32u), denominator);
}

static int test_empty_and_arguments(void)
{
    pr_u5e_2879_cpu cpu = frame();
    pr_u5e_2879_cpu before = cpu;
    CHECK(pr_u5e_party_value_2879(NULL, sizeof ram, &cpu, 1u) ==
          PR_U5E_2879_ARGUMENT);
    CHECK(pr_u5e_party_value_2879(ram, sizeof ram - 1u, &cpu, 1u) ==
          PR_U5E_2879_ARGUMENT);
    CHECK(pr_u5e_party_value_2879(ram, sizeof ram, NULL, 1u) ==
          PR_U5E_2879_ARGUMENT);
    CHECK(pr_u5e_party_value_2879(ram, sizeof ram, &cpu, 0u) ==
          PR_U5E_2879_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
    put8(cpu.ds, 0x6E2Fu, 0xA5u);
    CHECK(pr_u5e_party_value_2879(ram, sizeof ram, &cpu, 1u) ==
          PR_U5E_2879_OK);
    CHECK(get8(cpu.ds, 0x6E2Fu) == 0xA5u);
    CHECK(cpu.ax == 0u && cpu.dx == 0u && cpu.si == 0x9ABCu);
    CHECK(cpu.bp == 0x7777u && cpu.sp == 0x8004u);
    CHECK(cpu.cs == 0x3000u && cpu.ip == 0x7000u);
    return 0;
}

static int test_filtered_and_eligible(void)
{
    pr_u5e_2879_cpu cpu = frame();
    head(0x0100u, 0x4000u);
    node(0x0100u, 0x0300u, 0x4000u, 0u, 1u, 99u, 99u);
    node(0x0300u, 0u, 0u, 1u, 1u, 3u, 4u);
    CHECK(pr_u5e_party_value_2879(ram, sizeof ram, &cpu, 2u) ==
          PR_U5E_2879_OK);
    CHECK(get8(cpu.ds, 0x6E2Fu) == 75u); /* (20*3/4)*5 */
    CHECK(cpu.si == 15u && cpu.ax == 75u && cpu.dx == 0u);
    CHECK(cpu.di == 0x0300u && cpu.es == 0x4000u);
    return 0;
}

static int test_gate_and_mul_low_word(void)
{
    pr_u5e_2879_cpu cpu = frame();
    unsigned i;
    head(0x0100u, 0x4000u);
    node(0x0100u, 0u, 0u, 1u, 0u, 250u, 5u);
    CHECK(pr_u5e_party_value_2879(ram, sizeof ram, &cpu, 1u) ==
          PR_U5E_2879_OK);
    CHECK(get8(cpu.ds, 0x6E2Fu) == 0u); /* numerator gate was zero */

    cpu = frame();
    head(0x0100u, 0x4000u);
    for (i = 0u; i < 20u; ++i)
        node((uint16_t)(0x0100u + 0x0200u * i),
             i == 19u ? 0u : (uint16_t)(0x0100u + 0x0200u * (i + 1u)),
             i == 19u ? 0u : 0x4000u, 1u, 1u, 255u, 1u);
    CHECK(pr_u5e_party_value_2879(ram, sizeof ram, &cpu, 20u) ==
          PR_U5E_2879_OK);
    CHECK(cpu.si == (uint16_t)(((20u * 5100u) & 0xFFFFu) / 20u));
    CHECK(get8(cpu.ds, 0x6E2Fu) == (uint8_t)(cpu.si * 5u));
    return 0;
}

static int test_cycle_limit_and_return_alias(void)
{
    pr_u5e_2879_cpu cpu = frame();
    head(0x0100u, 0x4000u);
    node(0x0100u, 0x0100u, 0x4000u, 0u, 0u, 0u, 0u);
    CHECK(pr_u5e_party_value_2879(ram, sizeof ram, &cpu, 3u) ==
          PR_U5E_2879_LIMIT);
    CHECK(cpu.ip == 0x289Eu && cpu.bp == 0x7FFEu);

    cpu = frame();
    cpu.ss = 0x0EE3u;
    cpu.sp = 0x7FFFu;
    put16(cpu.ss, cpu.sp, 0x7000u);
    put16(cpu.ss, (uint16_t)(cpu.sp + 2u), cpu.cs);
    head(0x0100u, 0x4000u);
    node(0x0100u, 0u, 0u, 1u, 1u, 3u, 4u);
    CHECK(physical(cpu.ds, 0x6E2Fu) == physical(cpu.ss, cpu.sp));
    CHECK(pr_u5e_party_value_2879(ram, sizeof ram, &cpu, 1u) ==
          PR_U5E_2879_OK);
    CHECK(cpu.ip == 0x704Bu && cpu.cs == 0x3000u);
    return 0;
}

int main(void)
{
    if (test_empty_and_arguments() || test_filtered_and_eligible() ||
        test_gate_and_mul_low_word() || test_cycle_limit_and_return_alias())
        return 1;
    puts("u5E +2879 synthetic routes: 4 passed");
    return 0;
}
