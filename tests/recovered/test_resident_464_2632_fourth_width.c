#include "poolrad/recovered/resident_464_2632_fourth_width.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { PRIVATE_RAM_BYTES = 655360 };

static unsigned address(unsigned seg, unsigned off)
{
    return ((seg << 4) + (off & 0xFFFFu)) & 0xFFFFFu;
}

static void put_word(unsigned char *ram, unsigned seg, unsigned off,
                     unsigned value)
{
    ram[address(seg, off)] = (unsigned char)value;
    ram[address(seg, off + 1u)] = (unsigned char)(value >> 8);
}

static unsigned get_word(const unsigned char *ram, unsigned seg, unsigned off)
{
    return ram[address(seg, off)] |
        ((unsigned)ram[address(seg, off + 1u)] << 8);
}

static const pr_r464_2632_cpu pre_jl = {
    0x0C88, 0x2699, 21, 0, 13, 0, 0x2612, 8,
    0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246
};
static const pr_r464_2632_cpu pre_jge = {
    0x0C88, 0x26FB, 8, 0, 13, 0, 0x2612, 8,
    0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202
};

typedef struct { unsigned count, errors; } stage_check;

static void stages(uint16_t ip, const uint8_t *ram,
                   const pr_r464_2632_cpu *cpu, void *context)
{
    static const uint16_t expected[] = {
        0x269B, 0x269D, 0x269F, 0x26CD, 0x26F7, 0x26FB
    };
    stage_check *check = context;
    if (check->count >= 6u || ip != expected[check->count])
        ++check->errors;
    if (ip == 0x269Fu && cpu->flags != 0x7212u) ++check->errors;
    if (ip == 0x26F7u && (cpu->ax != 8u || cpu->flags != 0x7246u ||
                          get_word(ram, cpu->ss, 0x3E70u) != 8u))
        ++check->errors;
    if (ip == 0x26FBu && cpu->flags != 0x7202u) ++check->errors;
    ++check->count;
}

static void setup(unsigned char *ram, pr_r464_2632_cpu *cpu)
{
    memset(ram, 0xA5, PR_R464_2632_RAM_BYTES);
    *cpu = pre_jl;
    put_word(ram, cpu->ss, 0x3E86u, 8u);
    put_word(ram, cpu->ss, 0x3E88u, 0x3DE4u);
    put_word(ram, cpu->ss, 0x3E92u, 11u);
    put_word(ram, cpu->ss, 0x3E94u, 62u);
    put_word(ram, cpu->ss, 0x3E96u, 2u);
    put_word(ram, cpu->es, 10u, 21u);
}

static void assert_unsupported_unchanged(unsigned char *ram,
                                         unsigned char *before,
                                         pr_r464_2632_cpu *cpu)
{
    pr_r464_2632_cpu old = *cpu;
    memcpy(before, ram, PR_R464_2632_RAM_BYTES);
    assert(pr_r464_2632_fourth_width(
        ram, PR_R464_2632_RAM_BYTES, cpu, NULL, NULL) ==
        PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(ram, before, PR_R464_2632_RAM_BYTES) == 0);
    assert(memcmp(cpu, &old, sizeof(*cpu)) == 0);
}

static void test_synthetic(void)
{
    unsigned char *ram = malloc(PR_R464_2632_RAM_BYTES);
    unsigned char *before = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu;
    stage_check check = {0, 0};
    assert(ram != NULL && before != NULL);
    setup(ram, &cpu);
    assert(pr_r464_2632_fourth_width(NULL, PR_R464_2632_RAM_BYTES,
                                      &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_width(ram, PRIVATE_RAM_BYTES,
                                      &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    cpu.flags = 0x7202u; /* Nonzero compare selects another earlier arm. */
    assert_unsupported_unchanged(ram, before, &cpu);
    setup(ram, &cpu);
    put_word(ram, cpu.es, 10u, 12u); /* width below x + count: JAE falls. */
    cpu.ax = 12u;
    assert_unsupported_unchanged(ram, before, &cpu);
    setup(ram, &cpu);
    put_word(ram, cpu.ss, 0x3E94u, 0xFFFFu); /* Unmeasured negative row. */
    assert_unsupported_unchanged(ram, before, &cpu);
    setup(ram, &cpu);
    assert(pr_r464_2632_fourth_width(ram, PR_R464_2632_RAM_BYTES,
                                      &cpu, stages, &check) ==
           PR_R464_2632_OK);
    assert(check.count == 6u && check.errors == 0u);
    assert(memcmp(&cpu, &pre_jge, sizeof(cpu)) == 0);
    assert(get_word(ram, cpu.ss, 0x3E74u) == 0u);
    assert(get_word(ram, cpu.ss, 0x3E70u) == 8u);

    setup(ram, &cpu);
    put_word(ram, cpu.es, 10u, 13u);
    cpu.ax = 13u; /* Exact-fit width still takes JAE. */
    assert(pr_r464_2632_fourth_width(ram, PR_R464_2632_RAM_BYTES,
                                      &cpu, NULL, NULL) ==
           PR_R464_2632_OK);
    assert(cpu.ip == 0x26FBu && cpu.ax == 0u &&
           get_word(ram, cpu.ss, 0x3E70u) == 0u);
    free(before); free(ram);
}

static int load_private(const char *path, unsigned char *out)
{
    FILE *f = fopen(path, "rb");
    size_t n;
    if (f == NULL) return 0;
    n = fread(out, 1, PRIVATE_RAM_BYTES, f);
    if (fgetc(f) != EOF) { fclose(f); return 0; }
    fclose(f);
    return n == PRIVATE_RAM_BYTES;
}

typedef struct {
    const char *dir;
    unsigned char *expected_ram;
    unsigned count, errors;
} private_check;

static void private_stages(uint16_t ip, const uint8_t *ram,
                           const pr_r464_2632_cpu *cpu, void *context)
{
    static const pr_r464_2632_cpu expected_cpu[] = {
        {0x0C88, 0x269B, 21, 0, 13, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246},
        {0x0C88, 0x269D, 21, 0, 13, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246},
        {0x0C88, 0x269F, 21, 0, 13, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7212},
        {0x0C88, 0x26CD, 21, 0, 13, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7212},
        {0x0C88, 0x26F7, 8, 0, 13, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246},
        {0x0C88, 0x26FB, 8, 0, 13, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202}
    };
    private_check *check = context;
    char path[1024];
    if (check->count >= 6u || ip != expected_cpu[check->count].ip ||
        snprintf(path, sizeof(path), "%s/ram-%04x.bin", check->dir, ip) >=
            (int)sizeof(path) ||
        !load_private(path, check->expected_ram) ||
        memcmp(ram, check->expected_ram, PRIVATE_RAM_BYTES) != 0 ||
        memcmp(cpu, &expected_cpu[check->count], sizeof(*cpu)) != 0)
        ++check->errors;
    ++check->count;
}

static int test_replay(const char *pre_path, const char *next_path,
                       const char *stage_dir)
{
    unsigned char *ram = calloc(PR_R464_2632_RAM_BYTES, 1);
    unsigned char *next = malloc(PRIVATE_RAM_BYTES);
    pr_r464_2632_cpu cpu = pre_jl;
    unsigned differences = 0;
    private_check check = {stage_dir, malloc(PRIVATE_RAM_BYTES), 0, 0};
    int status;
    if (ram == NULL || next == NULL || check.expected_ram == NULL ||
        !load_private(pre_path, ram) ||
        !load_private(next_path, next)) {
        fprintf(stderr, "private RAM load failed\n");
        free(ram); free(next); free(check.expected_ram);
        return 1;
    }
    status = pr_r464_2632_fourth_width(
        ram, PR_R464_2632_RAM_BYTES, &cpu,
        stage_dir != NULL ? private_stages : NULL, &check);
    for (unsigned i = 0; i < PRIVATE_RAM_BYTES; ++i)
        differences += ram[i] != next[i];
    printf("fourth +2699..+26FB replay RAM mismatches: %u stages: %u errors: %u\n",
           differences, check.count, check.errors);
    status = status != PR_R464_2632_OK || differences != 0u ||
             memcmp(&cpu, &pre_jge, sizeof(cpu)) != 0 ||
             (stage_dir != NULL && (check.count != 6u || check.errors != 0u));
    free(ram); free(next); free(check.expected_ram);
    return status;
}

int main(int argc, char **argv)
{
    test_synthetic();
    if (argc == 3 || argc == 4)
        return test_replay(argv[1], argv[2], argc == 4 ? argv[3] : NULL);
    return argc == 1 ? 0 : 2;
}
