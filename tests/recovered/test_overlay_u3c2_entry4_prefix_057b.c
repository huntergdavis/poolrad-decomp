#include "poolrad/recovered/overlay_u3c2_entry4_prefix_057b.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t address(uint16_t ss, uint16_t offset)
{
    return (((size_t)ss * 16u) + offset) % PR_U3C2_057B_PREFIX_RAM_SIZE;
}

static uint16_t cmp_flags(uint8_t value)
{
    unsigned bit, ones = 0;
    uint16_t flags = 0;
    for (bit = 0; bit < 8u; ++bit) ones += (value >> bit) & 1u;
    if ((ones % 2u) == 0u) flags |= 4u;
    if (value == 0u) flags |= 0x40u;
    if (value >= 128u) flags |= 0x80u;
    return flags;
}

static void check(uint8_t *ram, uint8_t *expected, uint16_t ss,
                  uint16_t sp, uint8_t value, uint16_t flags)
{
    pr_u3c2_057b_prefix_cpu cpu = {
        0x1234, 0x2345, 0x3456, 0x4567, 0x5678, 0x6789,
        0x7788, sp, 0xD000, PR_U3C2_057B_PREFIX_ENTRY,
        0x1000, 0x3000, ss, flags
    };
    pr_u3c2_057b_prefix_cpu wanted = cpu;
    size_t saved = address(ss, (uint16_t)(sp - 2u));
    memset(ram, 0xA5, PR_U3C2_057B_PREFIX_RAM_SIZE);
    ram[address(ss, (uint16_t)(sp + 12u))] = value;
    memcpy(expected, ram, PR_U3C2_057B_PREFIX_RAM_SIZE);
    expected[saved] = 0x88;
    expected[(saved + 1u) % PR_U3C2_057B_PREFIX_RAM_SIZE] = 0x77;
    wanted.bp = (uint16_t)(sp - 2u);
    wanted.sp = (uint16_t)(sp - 6u);
    wanted.ip = PR_U3C2_057B_PREFIX_STOP;
    wanted.flags = (uint16_t)((flags & (uint16_t)~0x08D5u) | cmp_flags(value));
    assert(pr_u3c2_entry4_prefix_057b(ram, PR_U3C2_057B_PREFIX_RAM_SIZE, &cpu)
           == PR_U3C2_057B_PREFIX_OK);
    assert(memcmp(&cpu, &wanted, sizeof(cpu)) == 0);
    assert(memcmp(ram, expected, PR_U3C2_057B_PREFIX_RAM_SIZE) == 0);
}

int main(void)
{
    static const uint16_t segments[] = {0, 0x2000, 0xFFFF};
    static const uint16_t stacks[] = {0, 1, 2, 3, 4, 5, 0x0010, 0x0011, 0x0012, 0x0013, 0xFFF0, 0xFFF2,
        0xFFF3, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF};
    static const uint8_t values[] = {0, 1, 3, 0x7F, 0x80, 0x81, 0xFE, 0xFF};
    static const uint16_t flags[] = {0x0202, 0x0ED7, 0x0047, 0x0A93, 0x0602, 0x0002};
    uint8_t *ram = malloc(PR_U3C2_057B_PREFIX_RAM_SIZE);
    uint8_t *expected = malloc(PR_U3C2_057B_PREFIX_RAM_SIZE);
    pr_u3c2_057b_prefix_cpu cpu, before;
    size_t i, j, k;
    unsigned value;
    assert(ram && expected);
    for (value = 0; value < 256u; ++value)
        check(ram, expected, 0x2000, 0x8000, (uint8_t)value, 0x0ED7);
    for (i = 0; i < sizeof(flags) / sizeof(flags[0]); ++i)
        for (j = 0; j < sizeof(values) / sizeof(values[0]); ++j)
            check(ram, expected, 0x2000, 0x8000, values[j], flags[i]);
    for (i = 0; i < sizeof(segments) / sizeof(segments[0]); ++i)
        for (j = 0; j < sizeof(stacks) / sizeof(stacks[0]); ++j)
            for (k = 0; k < sizeof(values) / sizeof(values[0]); ++k)
                check(ram, expected, segments[i], stacks[j], values[k], 0x0ED7);

    memset(&cpu, 0, sizeof(cpu));
    cpu.ip = PR_U3C2_057B_PREFIX_ENTRY;
    before = cpu;
    memcpy(expected, ram, PR_U3C2_057B_PREFIX_RAM_SIZE);
    assert(pr_u3c2_entry4_prefix_057b(NULL, PR_U3C2_057B_PREFIX_RAM_SIZE, &cpu)
           == PR_U3C2_057B_PREFIX_ARGUMENT);
    assert(pr_u3c2_entry4_prefix_057b(ram, PR_U3C2_057B_PREFIX_RAM_SIZE, NULL)
           == PR_U3C2_057B_PREFIX_ARGUMENT);
    assert(pr_u3c2_entry4_prefix_057b(ram, PR_U3C2_057B_PREFIX_RAM_SIZE - 1u, &cpu)
           == PR_U3C2_057B_PREFIX_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof(cpu)) == 0);
    assert(memcmp(ram, expected, PR_U3C2_057B_PREFIX_RAM_SIZE) == 0);
    cpu.ip = PR_U3C2_057B_PREFIX_STOP;
    before = cpu;
    assert(pr_u3c2_entry4_prefix_057b(ram, PR_U3C2_057B_PREFIX_RAM_SIZE, &cpu)
           == PR_U3C2_057B_PREFIX_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof(cpu)) == 0);
    assert(memcmp(ram, expected, PR_U3C2_057B_PREFIX_RAM_SIZE) == 0);
    free(expected);
    free(ram);
    return 0;
}
