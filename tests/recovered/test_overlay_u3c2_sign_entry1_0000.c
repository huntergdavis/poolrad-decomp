#include "poolrad/recovered/overlay_u3c2_sign_entry1_0000.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(pr_u3c2_0000_cpu) == 28u, "14 words without padding");

static size_t address(uint16_t ss, unsigned offset)
{
    return ((size_t)ss * 16u + offset % 65536u) % PR_U3C2_0000_RAM_SIZE;
}

static void word(uint8_t *ram, uint16_t ss, unsigned offset, uint16_t value)
{
    size_t p = address(ss, offset);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) % PR_U3C2_0000_RAM_SIZE] = (uint8_t)(value / 256u);
}

static uint16_t sign_word(uint16_t value)
{
    int signed_value = value < 32768u ? (int)value : (int)value - 65536;
    return signed_value < 0 ? 65535u : (uint16_t)(signed_value > 0);
}

/* Independent final-state specification: parity belongs to the argument
 * CMP, not the sign result. Zero's XOR chooses AF=0 as a model policy. */
static uint16_t flags_for(uint16_t incoming, uint16_t value)
{
    unsigned ones = 0u;
    for (unsigned bit = 0u; bit < 8u; ++bit)
        ones += (value / (1u << bit)) % 2u;
    return (uint16_t)((incoming & 0xF72Au) |
                      (ones % 2u == 0u ? 4u : 0u) |
                      (value == 0u ? 64u : 0u) |
                      (value >= 32768u ? 128u : 0u));
}

static pr_u3c2_0000_cpu initial(uint16_t ss, uint16_t sp, uint16_t flags)
{
    pr_u3c2_0000_cpu cpu = {
        0xA55Au, 0x1357u, 0x2468u, 0x3579u, 0x468Au, 0x579Bu,
        0xBEEFu, sp, 0xF000u, 0u, 0x6000u, 0x8000u, ss, flags
    };
    return cpu;
}

static void check(uint8_t *ram, uint8_t *expected, uint16_t ss,
                  uint16_t sp, uint16_t value, uint16_t flags, int full_ram)
{
    pr_u3c2_0000_cpu actual = initial(ss, sp, flags);
    pr_u3c2_0000_cpu want = actual;
    word(ram, ss, sp, 0x0712u);
    word(ram, ss, (unsigned)sp + 2u, 0x4567u);
    word(ram, ss, (unsigned)sp + 4u, value);
    if (full_ram) {
        memcpy(expected, ram, PR_U3C2_0000_RAM_SIZE);
    } else {
        word(expected, ss, sp, 0x0712u);
        word(expected, ss, (unsigned)sp + 2u, 0x4567u);
        word(expected, ss, (unsigned)sp + 4u, value);
    }
    want.ax = sign_word(value);
    want.flags = flags_for(flags, value);
    want.sp = (uint16_t)((unsigned)sp + 6u);
    want.ip = 0x0712u;
    want.cs = 0x4567u;
    /* The complete function writes exactly the saved BP and local word. */
    word(expected, ss, (unsigned)sp + 65534u, want.bp);
    word(expected, ss, (unsigned)sp + 65532u, want.ax);
    assert(pr_u3c2_sign_entry1_0000(ram, PR_U3C2_0000_RAM_SIZE,
                                  &actual) == PR_U3C2_0000_OK);
    assert(memcmp(&actual, &want, sizeof(want)) == 0);
    if (full_ram)
        assert(memcmp(ram, expected, PR_U3C2_0000_RAM_SIZE) == 0);
}

static void rejects_atomically(uint8_t *ram, uint8_t *expected)
{
    pr_u3c2_0000_cpu cpu = initial(0x7000u, 0x8000u, 0xFFFFu);
    pr_u3c2_0000_cpu before = cpu;
    memcpy(expected, ram, PR_U3C2_0000_RAM_SIZE);
    assert(pr_u3c2_sign_entry1_0000(NULL, PR_U3C2_0000_RAM_SIZE, &cpu)
           == PR_U3C2_0000_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof(cpu)) == 0);
    assert(pr_u3c2_sign_entry1_0000(ram, PR_U3C2_0000_RAM_SIZE, NULL)
           == PR_U3C2_0000_ARGUMENT);
    assert(pr_u3c2_sign_entry1_0000(ram, 0u, &cpu)
           == PR_U3C2_0000_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof(cpu)) == 0);
    assert(pr_u3c2_sign_entry1_0000(ram, PR_U3C2_0000_RAM_SIZE - 1u, &cpu)
           == PR_U3C2_0000_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof(cpu)) == 0);
    cpu.ip = 1u;
    before = cpu;
    assert(pr_u3c2_sign_entry1_0000(ram, PR_U3C2_0000_RAM_SIZE, &cpu)
           == PR_U3C2_0000_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof(cpu)) == 0);
    assert(memcmp(ram, expected, PR_U3C2_0000_RAM_SIZE) == 0);
}

int main(void)
{
    static const uint16_t values[] = {
        0u, 1u, 2u, 0x0100u, 0x7FFFu, 0x8000u, 0xFF00u, 0xFFFFu
    };
    static const uint16_t stacks[] = {
        0u, 1u, 2u, 3u, 4u, 0x7FFFu, 0x8000u,
        0xFFFCu, 0xFFFDu, 0xFFFEu, 0xFFFFu
    };
    static const uint16_t segments[] = {0u, 0x2000u, 0x7000u, 0xFFFFu};
    uint8_t *ram = malloc(PR_U3C2_0000_RAM_SIZE);
    uint8_t *expected = malloc(PR_U3C2_0000_RAM_SIZE);
    unsigned count = 0u;
    uint32_t seed = 0xC3A50123u;
    assert(ram != NULL && expected != NULL);
    for (size_t i = 0u; i < PR_U3C2_0000_RAM_SIZE; ++i) {
        seed = seed * 1664525u + 1013904223u;
        ram[i] = (uint8_t)(seed >> 24);
    }
    memcpy(expected, ram, PR_U3C2_0000_RAM_SIZE);
    rejects_atomically(ram, expected);
    /* Literal flag expectations catch a mistaken parity-of-result model. */
    assert(flags_for(0x0AD7u, 0u) == 0x0246u);
    assert(flags_for(0x0AD7u, 1u) == 0x0202u);
    assert(flags_for(0x0AD7u, 2u) == 0x0202u);
    assert(flags_for(0x0AD7u, 0x0100u) == 0x0206u);
    assert(flags_for(0x0AD7u, 0x7FFFu) == 0x0206u);
    assert(flags_for(0x0AD7u, 0x8000u) == 0x0286u);
    assert(flags_for(0x0AD7u, 0xFF00u) == 0x0286u);
    assert(flags_for(0x0AD7u, 0xFFFFu) == 0x0286u);
    for (unsigned value = 0u; value < 65536u; ++value) {
        check(ram, expected, 0x7000u, 0x8000u, (uint16_t)value,
              0x0AD7u, 0);
        ++count;
    }
    assert(memcmp(ram, expected, PR_U3C2_0000_RAM_SIZE) == 0);
    for (unsigned flags = 0u; flags < 65536u; ++flags) {
        check(ram, expected, 0x7000u, 0x8000u, values[flags % 8u],
              (uint16_t)flags, 0);
        ++count;
    }
    assert(memcmp(ram, expected, PR_U3C2_0000_RAM_SIZE) == 0);
    for (size_t s = 0u; s < sizeof(segments) / sizeof(segments[0]); ++s)
        for (size_t p = 0u; p < sizeof(stacks) / sizeof(stacks[0]); ++p)
            for (size_t v = 0u; v < sizeof(values) / sizeof(values[0]); ++v) {
                check(ram, expected, segments[s], stacks[p], values[v],
                      (uint16_t)(0xA5D7u ^ (unsigned)(s * 91u + p)), 1);
                ++count;
            }
    /* Historical native f936bf4 proves separate RETF word-pop wrapping.
     * A linear four-byte frame would return ABCD instead of 4567 here. */
    for (unsigned sp = 0xFFFEu; sp <= 0xFFFFu; ++sp) {
        size_t shadow = (size_t)0x2000u * 16u + sp + 2u;
        ram[shadow] = 0xCDu;
        ram[shadow + 1u] = 0xABu;
        check(ram, expected, 0x2000u, (uint16_t)sp, 0x8000u, 0xFFFFu, 1);
        ++count;
    }
    free(expected);
    free(ram);
    printf("U3C2 +0000 synthetic PASS: %u CPU cases; 354 full-RAM edges; 5 atomic guards\n", count);
    return 0;
}
