#include "poolrad/recovered/resident_464_2632_fourth_row.h"

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

static const pr_r464_2632_cpu pre_jge = {
    0x0C88, 0x26FB, 8, 0, 13, 0, 0x2612, 8,
    0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202
};
static const pr_r464_2632_cpu pre_jl = {
    0x0C88, 0x275B, 168, 0, 104, 0, 0x2612, 8,
    0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246
};

typedef struct { unsigned count, errors; } stage_check;

static void stages(uint16_t ip, const uint8_t *ram,
                   const pr_r464_2632_cpu *cpu, void *context)
{
    static const uint16_t expected[] = {
        0x2722, 0x2728, 0x2732, 0x2735, 0x273C, 0x2751, 0x275B
    };
    stage_check *check = context;
    if (check->count >= 7u || ip != expected[check->count])
        ++check->errors;
    if (ip == 0x2732u &&
        (cpu->ax != 1302u || cpu->dx != 0u || cpu->flags != 0x7202u))
        ++check->errors;
    if (ip == 0x273Cu &&
        (cpu->ax != 41u || get_word(ram, cpu->ss, 0x3E6Cu) != 41u))
        ++check->errors;
    if (ip == 0x275Bu &&
        (cpu->ax != 168u || cpu->cx != 104u || cpu->flags != 0x7246u))
        ++check->errors;
    ++check->count;
}

static void setup(unsigned char *ram, pr_r464_2632_cpu *cpu)
{
    memset(ram, 0xA5, PR_R464_2632_RAM_BYTES);
    *cpu = pre_jge;
    put_word(ram, cpu->ss, 0x3E86u, 8u);
    put_word(ram, cpu->ss, 0x3E88u, 0x3DE4u);
    put_word(ram, cpu->ss, 0x3E90u, 42u);
    put_word(ram, cpu->ss, 0x3E92u, 11u);
    put_word(ram, cpu->ss, 0x3E94u, 62u);
    put_word(ram, cpu->ss, 0x3E96u, 2u);
    put_word(ram, cpu->es, 8u, 168u);
    put_word(ram, cpu->es, 10u, 21u);
}

static void assert_unsupported_unchanged(unsigned char *ram,
                                         unsigned char *before,
                                         pr_r464_2632_cpu *cpu)
{
    pr_r464_2632_cpu old = *cpu;
    memcpy(before, ram, PR_R464_2632_RAM_BYTES);
    assert(pr_r464_2632_fourth_row(ram, PR_R464_2632_RAM_BYTES,
                                    cpu, NULL, NULL) ==
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
    assert(pr_r464_2632_fourth_row(NULL, PR_R464_2632_RAM_BYTES,
                                    &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_row(ram, PRIVATE_RAM_BYTES,
                                    &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    cpu.flags = 0x7246u; /* Not the row-62 CMP result. */
    assert_unsupported_unchanged(ram, before, &cpu);
    setup(ram, &cpu);
    put_word(ram, cpu.ss, 0x3E94u, 0xFFFFu);
    assert_unsupported_unchanged(ram, before, &cpu);
    setup(ram, &cpu);
    put_word(ram, cpu.ss, 0x3E90u, 0u);
    assert_unsupported_unchanged(ram, before, &cpu);
    setup(ram, &cpu);
    put_word(ram, cpu.ss, 0x3E94u, 0x3000u); /* Product overflows. */
    assert_unsupported_unchanged(ram, before, &cpu);
    setup(ram, &cpu);
    assert(pr_r464_2632_fourth_row(ram, PR_R464_2632_RAM_BYTES,
                                    &cpu, stages, &check) == PR_R464_2632_OK);
    assert(check.count == 7u && check.errors == 0u);
    assert(memcmp(&cpu, &pre_jl, sizeof(cpu)) == 0);
    assert(get_word(ram, cpu.ss, 0x3E62u) == 62u);
    assert(get_word(ram, cpu.ss, 0x3E6Au) == 1302u);
    assert(get_word(ram, cpu.ss, 0x3E6Cu) == 41u);
    setup(ram, &cpu);
    put_word(ram, cpu.ss, 0x3E94u, 1u);
    put_word(ram, cpu.ss, 0x3E90u, 5u);
    put_word(ram, cpu.es, 8u, 120u);
    assert(pr_r464_2632_fourth_row(ram, PR_R464_2632_RAM_BYTES,
                                    &cpu, NULL, NULL) == PR_R464_2632_OK);
    assert(cpu.ip == 0x275Bu && cpu.ax == 120u && cpu.cx == 6u);
    assert(get_word(ram, cpu.ss, 0x3E6Au) == 21u);
    assert(get_word(ram, cpu.ss, 0x3E6Cu) == 4u);
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
        {0x0C88, 0x2722, 8, 0, 13, 0, 0x2612, 8, 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202},
        {0x0C88, 0x2728, 62, 0, 13, 0, 0x2612, 8, 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202},
        {0x0C88, 0x2732, 1302, 0, 13, 0, 0x2612, 8, 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202},
        {0x0C88, 0x2735, 1302, 0, 13, 0, 0x2612, 8, 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202},
        {0x0C88, 0x273C, 41, 0, 13, 0, 0x2612, 8, 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202},
        {0x0C88, 0x2751, 104, 0, 104, 0, 0x2612, 8, 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246},
        {0x0C88, 0x275B, 168, 0, 104, 0, 0x2612, 8, 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246}
    };
    private_check *check = context;
    char path[1024];
    if (check->count >= 7u || ip != expected_cpu[check->count].ip ||
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
    private_check check = {stage_dir, malloc(PRIVATE_RAM_BYTES), 0, 0};
    pr_r464_2632_cpu cpu = pre_jge;
    unsigned differences = 0;
    int status;
    if (ram == NULL || next == NULL || check.expected_ram == NULL ||
        !load_private(pre_path, ram) || !load_private(next_path, next)) {
        free(ram); free(next); free(check.expected_ram);
        return 1;
    }
    status = pr_r464_2632_fourth_row(
        ram, PR_R464_2632_RAM_BYTES, &cpu,
        stage_dir != NULL ? private_stages : NULL, &check);
    for (unsigned i = 0; i < PRIVATE_RAM_BYTES; ++i)
        differences += ram[i] != next[i];
    printf("fourth +26FB..+275B RAM mismatches: %u stages: %u errors: %u\n",
           differences, check.count, check.errors);
    status = status != PR_R464_2632_OK || differences != 0u ||
             memcmp(&cpu, &pre_jl, sizeof(cpu)) != 0 ||
             (stage_dir != NULL && (check.count != 7u || check.errors != 0u));
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
