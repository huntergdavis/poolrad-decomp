#include "poolrad/recovered/resident_464_second_postreturn_29cb.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { RAM_BYTES = PR_R464_SECOND_POSTRETURN_RAM_BYTES };

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

static int prepare(uint8_t *ram, int frozen)
{
    static const uint8_t continuation[] = {
        0x83, 0x46, 0xEA, 0x04, 0xA0, 0xC7, 0x70,
        0x3A, 0x46, 0xDC, 0x75, 0xD4
    };
    static const uint8_t call[] = {0x9A, 0x85, 0x16, 0x1C, 0x13};
    static const uint8_t move[] = {
        0x8B, 0xDC, 0x8C, 0xDA, 0x36, 0xC5, 0x77, 0x0A,
        0x36, 0xC4, 0x7F, 0x06, 0x36, 0x8B, 0x4F, 0x04,
        0xFC, 0x3B, 0xF7, 0x73, 0x07, 0x03, 0xF1, 0x03,
        0xF9, 0x4E, 0x4F, 0xFD, 0xF3, 0xA4, 0x8E, 0xDA,
        0xCA, 0x0A, 0x00
    };
    static const uint8_t mask[] = {0x00, 0xFF, 0xFF, 0x00};

    if (frozen) {
        /* The original loaded code comes from the run015 image. */
        if (memcmp(ram + physical(0x0C88u, 0x29CBu), continuation,
                   sizeof continuation) != 0 ||
            memcmp(ram + physical(0x0C88u, 0x29C6u), call,
                   sizeof call) != 0 ||
            memcmp(ram + physical(0x131Cu, 0x1685u), move,
                   sizeof move) != 0)
            return 0;
    } else {
        memcpy(ram + physical(0x0C88u, 0x29CBu), continuation,
               sizeof continuation);
        memcpy(ram + physical(0x0C88u, 0x29C6u), call, sizeof call);
        memcpy(ram + physical(0x131Cu, 0x1685u), move, sizeof move);
    }
    /* The second return is independently supplied, never produced by Move. */
    put_word(ram, 0x1D16u, 0x3E4Eu, 0x29CBu);
    put_word(ram, 0x1D16u, 0x3E50u, 0x0C88u);
    put_word(ram, 0x1D16u, 0x3E52u, 4u);
    put_word(ram, 0x1D16u, 0x3E54u, 0x1483u);
    put_word(ram, 0x1D16u, 0x3E56u, 0x3DE4u);
    put_word(ram, 0x1D16u, 0x3E58u, 0x3E64u);
    put_word(ram, 0x1D16u, 0x3E5Au, 0x1D16u);
    ram[physical(0x1D16u, 0x3E5Cu)] = 0x0Bu;
    put_word(ram, 0x1D16u, 0x3E6Au, 0x1464u);
    ram[physical(0x14A0u, 0x70C7u)] = 2u;
    memcpy(ram + physical(0x1D16u, 0x3E64u), mask, sizeof mask);
    memcpy(ram + physical(0x3DE4u, 0x147Fu), mask, sizeof mask);
    memcpy(ram + physical(0x3DE4u, 0x1483u), mask, sizeof mask);
    return 1;
}

static int rejected_unchanged(uint8_t *ram, uint8_t *before,
                              pr_r464_2632_cpu cpu)
{
    pr_r464_2632_cpu saved = cpu;
    memcpy(before, ram, RAM_BYTES);
    return pr_r464_second_postreturn_29cb(ram, RAM_BYTES, &cpu) ==
               PR_R464_SECOND_POSTRETURN_UNSUPPORTED_PATH &&
           memcmp(&cpu, &saved, sizeof cpu) == 0 &&
           memcmp(ram, before, RAM_BYTES) == 0;
}

static int run_case(uint8_t *ram, int frozen)
{
    uint8_t *before = malloc(RAM_BYTES);
    pr_r464_2632_cpu cpu = {
        0x0C88, 0x29CB, 4, 0x3E4E, 0, 0x14A0, 0x3E68, 0x1487,
        0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7206
    };
    pr_r464_2632_cpu expected = cpu;
    int ok = 0;

    if (before == NULL || !prepare(ram, frozen)) goto done;
    if (pr_r464_second_postreturn_29cb(NULL, RAM_BYTES, &cpu) !=
            PR_R464_SECOND_POSTRETURN_INVALID_HOST ||
        pr_r464_second_postreturn_29cb(ram, RAM_BYTES, NULL) !=
            PR_R464_SECOND_POSTRETURN_INVALID_HOST ||
        pr_r464_second_postreturn_29cb(ram, RAM_BYTES - 1u, &cpu) !=
            PR_R464_SECOND_POSTRETURN_INVALID_HOST)
        goto done;
    ram[physical(0x0C88u, 0x29CBu)] ^= 1u;
    if (!rejected_unchanged(ram, before, cpu)) goto done;
    ram[physical(0x0C88u, 0x29CBu)] ^= 1u;
    ram[physical(0x0C88u, 0x29C6u)] ^= 1u;
    if (!rejected_unchanged(ram, before, cpu)) goto done;
    ram[physical(0x0C88u, 0x29C6u)] ^= 1u;
    ram[physical(0x131Cu, 0x1685u)] ^= 1u;
    if (!rejected_unchanged(ram, before, cpu)) goto done;
    ram[physical(0x131Cu, 0x1685u)] ^= 1u;
    ram[physical(0x1D16u, 0x3E5Cu)] ^= 1u;
    if (!rejected_unchanged(ram, before, cpu)) goto done;
    ram[physical(0x1D16u, 0x3E5Cu)] ^= 1u;
    ram[physical(0x3DE4u, 0x1483u)] ^= 1u;
    if (!rejected_unchanged(ram, before, cpu)) goto done;
    ram[physical(0x3DE4u, 0x1483u)] ^= 1u;
    expected.flags ^= 1u;
    if (!rejected_unchanged(ram, before, expected)) goto done;

    memcpy(before, ram, RAM_BYTES);
    put_word(before, 0x1D16u, 0x3E6Au, 0x1468u);
    expected = cpu;
    expected.ax = 2u;
    expected.ip = 0x29ABu;
    expected.flags = 0x7293u;
    if (pr_r464_second_postreturn_29cb(ram, RAM_BYTES, &cpu) !=
            PR_R464_SECOND_POSTRETURN_OK ||
        memcmp(&cpu, &expected, sizeof cpu) != 0 ||
        memcmp(ram, before, RAM_BYTES) != 0)
        goto done;
    ok = 1;
done:
    free(before);
    return ok;
}

int main(int argc, char **argv)
{
    uint8_t *ram = calloc(RAM_BYTES, 1u);
    FILE *file = NULL;
    int ok = 0;

    if (ram == NULL || !run_case(ram, 0)) goto done;
    if (argc == 2) {
        file = fopen(argv[1], "rb");
        if (file == NULL || fread(ram, 1u, RAM_BYTES, file) != RAM_BYTES ||
            fgetc(file) != EOF || !run_case(ram, 1))
            goto done;
    } else if (argc != 1) {
        goto done;
    }
    ok = 1;
done:
    if (file != NULL) fclose(file);
    free(ram);
    if (!ok) fputs("resident_464_second_postreturn_29cb: FAIL\n", stderr);
    else puts("resident_464_second_postreturn_29cb: PASS");
    return ok ? 0 : 1;
}
