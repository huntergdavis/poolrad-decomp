#include "poolrad/recovered/resident_464_fourth_branch_275b.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const uint8_t branch_code[] = {0x7c, 0x06, 0x7f, 0x1e,
                                      0x3b, 0xc1, 0x73, 0x1a};

static pr_r464_2632_cpu native_start(void)
{
    const pr_r464_2632_cpu cpu = {
        0x0C88, 0x275B, 0x00A8, 0, 0x0068, 0, 0x2612, 0x0008,
        0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246
    };
    return cpu;
}

static void test_synthetic(void)
{
    uint8_t *ram = calloc(PR_R464_FOURTH_RAM_BYTES, 1u);
    uint8_t *before = malloc(PR_R464_FOURTH_RAM_BYTES);
    pr_r464_2632_cpu cpu = native_start(), old;
    const size_t code_at = (0x0C88u << 4) + 0x275Bu;
    assert(ram != NULL && before != NULL);
    memcpy(ram + code_at, branch_code, sizeof branch_code);
    ram[0x22000u] = 0xA5u;
    memcpy(before, ram, PR_R464_FOURTH_RAM_BYTES);

    assert(pr_r464_fourth_branch_275b(NULL, PR_R464_FOURTH_RAM_BYTES,
                                     &cpu, NULL, NULL) ==
           PR_R464_FOURTH_INVALID_HOST);
    assert(pr_r464_fourth_branch_275b(ram, PR_R464_FOURTH_RAM_BYTES - 1u,
                                     &cpu, NULL, NULL) ==
           PR_R464_FOURTH_INVALID_HOST);
    cpu.flags &= (uint16_t)~0x40u;
    old = cpu;
    assert(pr_r464_fourth_branch_275b(ram, PR_R464_FOURTH_RAM_BYTES,
                                     &cpu, NULL, NULL) ==
           PR_R464_FOURTH_UNSUPPORTED_PATH);
    assert(memcmp(&cpu, &old, sizeof cpu) == 0);
    cpu = native_start();
    cpu.ax = 0x0067u;
    old = cpu;
    assert(pr_r464_fourth_branch_275b(ram, PR_R464_FOURTH_RAM_BYTES,
                                     &cpu, NULL, NULL) ==
           PR_R464_FOURTH_UNSUPPORTED_PATH);
    assert(memcmp(&cpu, &old, sizeof cpu) == 0);
    cpu = native_start();
    ram[code_at] ^= 1u;
    old = cpu;
    assert(pr_r464_fourth_branch_275b(ram, PR_R464_FOURTH_RAM_BYTES,
                                     &cpu, NULL, NULL) ==
           PR_R464_FOURTH_UNSUPPORTED_PATH);
    assert(memcmp(&cpu, &old, sizeof cpu) == 0);
    ram[code_at] ^= 1u;
    assert(memcmp(ram, before, PR_R464_FOURTH_RAM_BYTES) == 0);

    assert(pr_r464_fourth_branch_275b(ram, PR_R464_FOURTH_RAM_BYTES,
                                     &cpu, NULL, NULL) == PR_R464_FOURTH_OK);
    assert(cpu.cs == 0x0C88u && cpu.ip == 0x277Du);
    assert(cpu.ax == 0x00A8u && cpu.cx == 0x0068u && cpu.flags == 0x7202u);
    assert(memcmp(ram, before, PR_R464_FOURTH_RAM_BYTES) == 0);
    free(before);
    free(ram);
}

typedef struct {
    uint16_t ip;
    uint16_t flags;
    const char *file;
} checkpoint;

static const checkpoint native_stops[] = {
    {0x275B, 0x7246, "ram-275b-fourth-res-jl-pre.bin"},
    {0x275D, 0x7246, "ram-275d-fourth-res-jl-fallthrough.bin"},
    {0x275F, 0x7246, "ram-275f-fourth-res-jg-fallthrough.bin"},
    {0x2761, 0x7202, "ram-2761-fourth-res-jae-pre.bin"},
    {0x277D, 0x7202, "ram-277d-fourth-res-jae-taken.bin"}
};

typedef struct {
    const char *directory;
    uint8_t *expected;
    unsigned seen;
    unsigned errors;
} native_context;

static int load_ram(const char *path, uint8_t *ram)
{
    FILE *file = fopen(path, "rb");
    size_t bytes;
    int extra;
    if (file == NULL) return 0;
    bytes = fread(ram, 1u, PR_R464_FOURTH_RAM_BYTES, file);
    extra = fgetc(file);
    fclose(file);
    return bytes == PR_R464_FOURTH_RAM_BYTES && extra == EOF;
}

static void compare_stop(uint16_t ip, const uint8_t *ram,
                         const pr_r464_2632_cpu *cpu, void *opaque)
{
    native_context *context = opaque;
    pr_r464_2632_cpu expected_cpu = native_start();
    const checkpoint *stop;
    char path[1024];
    if (context->seen >= sizeof native_stops / sizeof native_stops[0]) {
        ++context->errors;
        return;
    }
    stop = &native_stops[context->seen++];
    expected_cpu.ip = stop->ip;
    expected_cpu.flags = stop->flags;
    if (ip != stop->ip || memcmp(cpu, &expected_cpu, sizeof *cpu) != 0) {
        fprintf(stderr, "CPU mismatch at +%04X\n", stop->ip);
        ++context->errors;
    }
    if (snprintf(path, sizeof path, "%s/%s", context->directory,
                 stop->file) >= (int)sizeof path ||
        !load_ram(path, context->expected) ||
        memcmp(ram, context->expected, PR_R464_FOURTH_RAM_BYTES) != 0) {
        fprintf(stderr, "full RAM mismatch at +%04X\n", stop->ip);
        ++context->errors;
    }
}

static int test_private(const char *directory)
{
    uint8_t *ram = malloc(PR_R464_FOURTH_RAM_BYTES);
    uint8_t *expected = malloc(PR_R464_FOURTH_RAM_BYTES);
    pr_r464_2632_cpu cpu = native_start();
    native_context context = {directory, expected, 0u, 0u};
    char path[1024];
    int failed;
    if (ram == NULL || expected == NULL) {
        free(ram);
        free(expected);
        return 1;
    }
    if (snprintf(path, sizeof path, "%s/%s", directory,
                 native_stops[0].file) >= (int)sizeof path ||
        !load_ram(path, ram)) {
        fprintf(stderr, "native entry RAM load failed\n");
        free(ram);
        free(expected);
        return 1;
    }
    failed = pr_r464_fourth_branch_275b(ram, PR_R464_FOURTH_RAM_BYTES,
                                        &cpu, compare_stop, &context) !=
             PR_R464_FOURTH_OK;
    if (context.seen != sizeof native_stops / sizeof native_stops[0])
        failed = 1;
    failed |= context.errors != 0u;
    free(ram);
    free(expected);
    return failed;
}

int main(int argc, char **argv)
{
    test_synthetic();
    if (argc == 1) return 0;
    if (argc != 2) return 2;
    return test_private(argv[1]);
}
