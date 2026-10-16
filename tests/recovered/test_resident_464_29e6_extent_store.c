#include "poolrad/recovered/resident_464_29e6_extent_store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        0x0C88, 0x29E6, 0x138F, 0x3E4E, 0, 0x14A0, 0x3E68, 0x0076,
        0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202
    };
    return cpu;
}

static void prepare(uint8_t *ram)
{
    static const uint8_t code[] = {
        0x8B, 0x46, 0xEA, 0x48, 0x8B, 0x7E, 0xE2,
        0xD1, 0xE7, 0x89, 0x85, 0x70, 0x4D
    };
    memset(ram, 0, PR_R464_29E6_RAM_BYTES);
    memcpy(ram + physical(0x0C88u, 0x29E6u), code, sizeof code);
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
}

static int rejected_unchanged(uint8_t *ram, uint8_t *before,
                              pr_r464_2632_cpu cpu)
{
    pr_r464_2632_cpu saved = cpu;
    memcpy(before, ram, PR_R464_29E6_RAM_BYTES);
    return pr_r464_29e6_extent_store(ram, PR_R464_29E6_RAM_BYTES, &cpu) ==
               PR_R464_29E6_UNSUPPORTED_PATH &&
           memcmp(&cpu, &saved, sizeof cpu) == 0 &&
           memcmp(ram, before, PR_R464_29E6_RAM_BYTES) == 0;
}

int main(void)
{
    uint8_t *ram = malloc(PR_R464_29E6_RAM_BYTES);
    uint8_t *before = malloc(PR_R464_29E6_RAM_BYTES);
    pr_r464_2632_cpu cpu = supplied_cpu();
    pr_r464_2632_cpu expected = cpu;
    size_t target = physical(0x14A0u, 0x4DE6u);
    int ok = ram != NULL && before != NULL;
    if (ok) {
        prepare(ram);
        ok = pr_r464_29e6_extent_store(NULL, PR_R464_29E6_RAM_BYTES,
                                       &cpu) == PR_R464_29E6_INVALID_HOST &&
             pr_r464_29e6_extent_store(ram, PR_R464_29E6_RAM_BYTES,
                                       NULL) == PR_R464_29E6_INVALID_HOST &&
             pr_r464_29e6_extent_store(ram, PR_R464_29E6_RAM_BYTES - 1u,
                                       &cpu) == PR_R464_29E6_INVALID_HOST;
        ram[physical(0x0C88u, 0x29E6u)] ^= 1u;
        ok = ok && rejected_unchanged(ram, before, cpu);
        ram[physical(0x0C88u, 0x29E6u)] ^= 1u;
        ram[physical(0x1D16u, 0x3E62u)] ^= 1u;
        ok = ok && rejected_unchanged(ram, before, cpu);
        ram[physical(0x1D16u, 0x3E62u)] ^= 1u;
        ram[target] ^= 1u;
        ok = ok && rejected_unchanged(ram, before, cpu);
        ram[target] ^= 1u;
        expected.flags ^= 1u;
        ok = ok && rejected_unchanged(ram, before, expected);

        memcpy(before, ram, PR_R464_29E6_RAM_BYTES);
        expected = cpu;
        expected.ip = 0x29F3u;
        if (pr_r464_29e6_extent_store(ram, PR_R464_29E6_RAM_BYTES,
                                      &cpu) != PR_R464_29E6_OK ||
            memcmp(&cpu, &expected, sizeof cpu) != 0 ||
            ram[target] != 0x8Fu || ram[target + 1u] != 0x13u)
            ok = 0;
        before[target] = 0x8Fu;
        before[target + 1u] = 0x13u;
        if (memcmp(ram, before, PR_R464_29E6_RAM_BYTES) != 0)
            ok = 0;
    }
    free(before);
    free(ram);
    if (!ok) fputs("resident_464_29e6_extent_store: FAIL\n", stderr);
    else puts("resident_464_29e6_extent_store: PASS");
    return ok ? 0 : 1;
}
