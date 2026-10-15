#include "poolrad/recovered/resident_464_2632_fourth_jl.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { PRIVATE_RAM_BYTES = 655360 };

static const pr_r464_2632_cpu pre_jl = {
    0x0C88, 0x275B, 168, 0, 104, 0, 0x2612, 8,
    0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246
};

typedef struct { unsigned count, errors; uint16_t expected; } stage_check;

static void stage(uint16_t ip, const uint8_t *ram,
                  const pr_r464_2632_cpu *cpu, void *context)
{
    stage_check *check = context;
    (void)ram;
    if (ip != check->expected || cpu->ip != ip) ++check->errors;
    ++check->count;
}

static void test_synthetic(void)
{
    uint8_t *ram = malloc(PR_R464_2632_RAM_BYTES);
    uint8_t *before = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu, old;
    stage_check check = {0, 0, 0x275D};
    assert(ram != NULL && before != NULL);
    memset(ram, 0xA5, PR_R464_2632_RAM_BYTES);
    memcpy(before, ram, PR_R464_2632_RAM_BYTES);
    cpu = pre_jl;
    assert(pr_r464_2632_fourth_jl(NULL, PR_R464_2632_RAM_BYTES,
                                   &cpu, NULL, NULL) == PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_jl(ram, PRIVATE_RAM_BYTES,
                                   &cpu, NULL, NULL) == PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_jl(ram, PR_R464_2632_RAM_BYTES,
                                   NULL, NULL, NULL) == PR_R464_2632_INVALID_HOST);
    cpu.ip = 0x275Du;
    old = cpu;
    assert(pr_r464_2632_fourth_jl(ram, PR_R464_2632_RAM_BYTES,
                                   &cpu, NULL, NULL) == PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(&cpu, &old, sizeof(cpu)) == 0);
    cpu = pre_jl;
    assert(pr_r464_2632_fourth_jl(ram, PR_R464_2632_RAM_BYTES,
                                   &cpu, stage, &check) == PR_R464_2632_OK);
    assert(cpu.ip == 0x275Du && cpu.flags == 0x7246u);
    assert(check.count == 1u && check.errors == 0u);
    assert(memcmp(ram, before, PR_R464_2632_RAM_BYTES) == 0);
    cpu = pre_jl;
    cpu.flags = 0x72C6u; /* SF=1, OF=0: JL taken. */
    check.count = check.errors = 0u;
    check.expected = 0x2763u;
    assert(pr_r464_2632_fourth_jl(ram, PR_R464_2632_RAM_BYTES,
                                   &cpu, stage, &check) == PR_R464_2632_OK);
    assert(cpu.ip == 0x2763u && cpu.flags == 0x72C6u);
    assert(check.count == 1u && check.errors == 0u);
    cpu = pre_jl;
    cpu.flags = 0x7AC6u; /* SF=OF=1: JL falls through. */
    assert(pr_r464_2632_fourth_jl(ram, PR_R464_2632_RAM_BYTES,
                                   &cpu, NULL, NULL) == PR_R464_2632_OK);
    assert(cpu.ip == 0x275Du && cpu.flags == 0x7AC6u);
    assert(memcmp(ram, before, PR_R464_2632_RAM_BYTES) == 0);
    free(before); free(ram);
}

static int load_private(const char *path, uint8_t *out)
{
    FILE *f = fopen(path, "rb");
    size_t n;
    if (f == NULL) return 0;
    n = fread(out, 1, PRIVATE_RAM_BYTES, f);
    if (fgetc(f) != EOF) { fclose(f); return 0; }
    fclose(f);
    return n == PRIVATE_RAM_BYTES;
}

static int test_replay(const char *pre_path)
{
    uint8_t *ram = calloc(PR_R464_2632_RAM_BYTES, 1);
    uint8_t *before = malloc(PRIVATE_RAM_BYTES);
    pr_r464_2632_cpu cpu = pre_jl;
    pr_r464_2632_cpu expected = pre_jl;
    stage_check check = {0, 0, 0x275D};
    int status;
    if (ram == NULL || before == NULL ||
        !load_private(pre_path, ram)) { free(ram); free(before); return 1; }
    memcpy(before, ram, PRIVATE_RAM_BYTES);
    status = pr_r464_2632_fourth_jl(
        ram, PR_R464_2632_RAM_BYTES, &cpu, stage, &check);
    printf("fourth +275B JL replay RAM mismatches: %d stages: %u errors: %u\n",
           memcmp(ram, before, PRIVATE_RAM_BYTES) != 0,
           check.count, check.errors);
    expected.ip = 0x275Du;
    status = status != PR_R464_2632_OK ||
             memcmp(&cpu, &expected, sizeof(cpu)) != 0 ||
             check.count != 1u || check.errors != 0u ||
             memcmp(ram, before, PRIVATE_RAM_BYTES) != 0;
    free(ram); free(before);
    return status;
}

int main(int argc, char **argv)
{
    test_synthetic();
    if (argc == 2) return test_replay(argv[1]);
    return argc == 1 ? 0 : 2;
}
