#include "poolrad/recovered/resident_464_third_return_29cb.h"

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
        0x0C88, 0x29CB, 4, 0x3E4E, 0, 0x14A0, 0x3E68, 0x148B,
        0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7216
    };
    return cpu;
}

static void prepare(uint8_t *ram)
{
    static const uint8_t call[] = {0x9A, 0x85, 0x16, 0x1C, 0x13};
    static const uint8_t move[] = {
        0x8B, 0xDC, 0x8C, 0xDA, 0x36, 0xC5, 0x77, 0x0A,
        0x36, 0xC4, 0x7F, 0x06, 0x36, 0x8B, 0x4F, 0x04,
        0xFC, 0x3B, 0xF7, 0x73, 0x07, 0x03, 0xF1, 0x03,
        0xF9, 0x4E, 0x4F, 0xFD, 0xF3, 0xA4, 0x8E, 0xDA,
        0xCA, 0x0A, 0x00
    };
    static const uint8_t mask[] = {0x00, 0xFF, 0xFF, 0x00};
    memset(ram, 0, PR_R464_THIRD_RETURN_RAM_BYTES);
    memcpy(ram + physical(0x0C88u, 0x29C6u), call, sizeof call);
    memcpy(ram + physical(0x131Cu, 0x1685u), move, sizeof move);
    memcpy(ram + physical(0x1D16u, 0x3E64u), mask, sizeof mask);
    memcpy(ram + physical(0x3DE4u, 0x147Fu), mask, sizeof mask);
    memcpy(ram + physical(0x3DE4u, 0x1483u), mask, sizeof mask);
    memcpy(ram + physical(0x3DE4u, 0x1487u), mask, sizeof mask);
    put_word(ram, 0x1D16u, 0x3E4Eu, 0x29CBu);
    put_word(ram, 0x1D16u, 0x3E50u, 0x0C88u);
    put_word(ram, 0x1D16u, 0x3E52u, 4u);
    put_word(ram, 0x1D16u, 0x3E54u, 0x1487u);
    put_word(ram, 0x1D16u, 0x3E56u, 0x3DE4u);
    put_word(ram, 0x1D16u, 0x3E58u, 0x3E64u);
    put_word(ram, 0x1D16u, 0x3E5Au, 0x1D16u);
    put_word(ram, 0x1D16u, 0x3E6Au, 0x1468u);
    ram[physical(0x14A0u, 0x70C7u)] = 3u;
}

static int check(uint8_t *ram, uint8_t *saved, pr_r464_2632_cpu cpu,
                 pr_r464_third_return_status wanted)
{
    pr_r464_2632_cpu before = cpu;
    memcpy(saved, ram, PR_R464_THIRD_RETURN_RAM_BYTES);
    return pr_r464_third_return_29cb(ram, PR_R464_THIRD_RETURN_RAM_BYTES,
                                    &cpu) == wanted &&
           memcmp(&cpu, &before, sizeof cpu) == 0 &&
           memcmp(ram, saved, PR_R464_THIRD_RETURN_RAM_BYTES) == 0;
}

int main(void)
{
    uint8_t *ram = malloc(PR_R464_THIRD_RETURN_RAM_BYTES);
    uint8_t *saved = malloc(PR_R464_THIRD_RETURN_RAM_BYTES);
    pr_r464_2632_cpu cpu = supplied_cpu();
    int ok = ram != NULL && saved != NULL;
    if (ok) {
        prepare(ram);
        ok = pr_r464_third_return_29cb(NULL, PR_R464_THIRD_RETURN_RAM_BYTES,
                                       &cpu) == PR_R464_THIRD_RETURN_INVALID_HOST &&
             pr_r464_third_return_29cb(ram, PR_R464_THIRD_RETURN_RAM_BYTES,
                                       NULL) == PR_R464_THIRD_RETURN_INVALID_HOST &&
             pr_r464_third_return_29cb(ram, PR_R464_THIRD_RETURN_RAM_BYTES - 1u,
                                       &cpu) == PR_R464_THIRD_RETURN_INVALID_HOST &&
             check(ram, saved, cpu, PR_R464_THIRD_RETURN_OK);
        ram[physical(0x3DE4u, 0x1487u) + 1u] ^= 1u;
        ok = ok && check(ram, saved, cpu, PR_R464_THIRD_RETURN_UNSUPPORTED_PATH);
        ram[physical(0x3DE4u, 0x1487u) + 1u] ^= 1u;
        ram[physical(0x131Cu, 0x1685u)] ^= 1u;
        ok = ok && check(ram, saved, cpu, PR_R464_THIRD_RETURN_UNSUPPORTED_PATH);
        ram[physical(0x131Cu, 0x1685u)] ^= 1u;
        ram[physical(0x14A0u, 0x70C7u)] ^= 1u;
        ok = ok && check(ram, saved, cpu, PR_R464_THIRD_RETURN_UNSUPPORTED_PATH);
        ram[physical(0x14A0u, 0x70C7u)] ^= 1u;
        cpu.flags ^= 1u;
        ok = ok && check(ram, saved, cpu, PR_R464_THIRD_RETURN_UNSUPPORTED_PATH);
    }
    free(saved);
    free(ram);
    if (!ok) fputs("resident_464_third_return_29cb: FAIL\n", stderr);
    else puts("resident_464_third_return_29cb: PASS");
    return ok ? 0 : 1;
}
