#include "poolrad/recovered/resident_464_29d7_extent_branch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { RAM_BYTES = PR_R464_29D7_RAM_BYTES };

static size_t physical(uint16_t segment, uint16_t offset)
{
    return ((size_t)segment << 4) + offset;
}

static void put_word(uint8_t *ram, uint16_t segment, uint16_t offset,
                     uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[at + 1u] = (uint8_t)(value >> 8);
}

static pr_r464_2632_cpu supplied_cpu(void)
{
    const pr_r464_2632_cpu cpu = {
        0x0C88, 0x29D7, 0x000B, 0x3E4E, 0, 0x14A0, 0x3E68, 0x13AF,
        0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246
    };
    return cpu;
}

static void prepare(uint8_t *ram, uint16_t extent)
{
    static const uint8_t code[] = {
        0x8B, 0x46, 0xEA, 0x48, 0x8B, 0x7E, 0xE2, 0xD1,
        0xE7, 0x3B, 0x85, 0x70, 0x4D, 0x76, 0x0D
    };
    memset(ram, 0, RAM_BYTES);
    memcpy(ram + physical(0x0C88u, 0x29D7u), code, sizeof code);
    put_word(ram, 0x1D16u, 0x3E4Eu, 0x29CBu);
    put_word(ram, 0x1D16u, 0x3E50u, 0x0C88u);
    put_word(ram, 0x1D16u, 0x3E52u, 4u);
    put_word(ram, 0x1D16u, 0x3E54u, 0x13ABu);
    put_word(ram, 0x1D16u, 0x3E56u, 0x3DE4u);
    put_word(ram, 0x1D16u, 0x3E58u, 0x3E64u);
    put_word(ram, 0x1D16u, 0x3E5Au, 0x1D16u);
    ram[physical(0x1D16u, 0x3E5Cu)] = 0x0Bu;
    put_word(ram, 0x1D16u, 0x3E62u, 0x003Bu);
    put_word(ram, 0x1D16u, 0x3E6Au, 0x1390u);
    ram[physical(0x14A0u, 0x70C7u)] = 0x0Bu;
    put_word(ram, 0x14A0u, 0x4DE6u, extent);
}

static int rejected_unchanged(uint8_t *ram, uint8_t *before,
                              pr_r464_2632_cpu cpu)
{
    pr_r464_2632_cpu saved = cpu;
    memcpy(before, ram, RAM_BYTES);
    return pr_r464_29d7_extent_branch(ram, RAM_BYTES, &cpu) ==
               PR_R464_29D7_UNSUPPORTED_PATH &&
           memcmp(&cpu, &saved, sizeof cpu) == 0 &&
           memcmp(ram, before, RAM_BYTES) == 0;
}

static int run_case(uint8_t *ram, uint8_t *before, uint16_t extent,
                    uint16_t expected_ip, uint16_t expected_flags)
{
    pr_r464_2632_cpu cpu = supplied_cpu();
    pr_r464_2632_cpu expected = cpu;

    prepare(ram, extent);
    if (pr_r464_29d7_extent_branch(NULL, RAM_BYTES, &cpu) !=
            PR_R464_29D7_INVALID_HOST ||
        pr_r464_29d7_extent_branch(ram, RAM_BYTES, NULL) !=
            PR_R464_29D7_INVALID_HOST ||
        pr_r464_29d7_extent_branch(ram, RAM_BYTES - 1u, &cpu) !=
            PR_R464_29D7_INVALID_HOST)
        return 0;
    ram[physical(0x0C88u, 0x29D7u)] ^= 1u;
    if (!rejected_unchanged(ram, before, cpu)) return 0;
    ram[physical(0x0C88u, 0x29D7u)] ^= 1u;
    ram[physical(0x1D16u, 0x3E5Cu)] ^= 1u;
    if (!rejected_unchanged(ram, before, cpu)) return 0;
    ram[physical(0x1D16u, 0x3E5Cu)] ^= 1u;
    ram[physical(0x1D16u, 0x3E6Au)] ^= 1u;
    if (!rejected_unchanged(ram, before, cpu)) return 0;
    ram[physical(0x1D16u, 0x3E6Au)] ^= 1u;
    ram[physical(0x1D16u, 0x3E4Eu)] ^= 1u;
    if (!rejected_unchanged(ram, before, cpu)) return 0;
    ram[physical(0x1D16u, 0x3E4Eu)] ^= 1u;
    expected.flags ^= 1u;
    if (!rejected_unchanged(ram, before, expected)) return 0;

    memcpy(before, ram, RAM_BYTES);
    expected = cpu;
    expected.ax = 0x138Fu;
    expected.di = 0x0076u;
    expected.flags = expected_flags;
    expected.ip = expected_ip;
    return pr_r464_29d7_extent_branch(ram, RAM_BYTES, &cpu) ==
               PR_R464_29D7_OK &&
           memcmp(&cpu, &expected, sizeof cpu) == 0 &&
           memcmp(ram, before, RAM_BYTES) == 0;
}

int main(void)
{
    uint8_t *ram = malloc(RAM_BYTES);
    uint8_t *before = malloc(RAM_BYTES);
    int ok = ram != NULL && before != NULL &&
             run_case(ram, before, 0x0000u, 0x29E6u, 0x7202u) &&
             run_case(ram, before, 0x138Fu, 0x29F3u, 0x7246u) &&
             run_case(ram, before, 0x1400u, 0x29F3u, 0x7283u);
    free(before);
    free(ram);
    if (!ok) fputs("resident_464_29d7_extent_branch: FAIL\n", stderr);
    else puts("resident_464_29d7_extent_branch: PASS");
    return ok ? 0 : 1;
}
