#include "poolrad/recovered/resident_464_2632_fourth_positive.h"

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

static const pr_r464_2632_cpu positive_pre = {
    0x0C88, 0x2656, 0x3DEC, 0x3E4E, 0, 0x14A0, 0x2612,
    0x3E80, 0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7202
};
static const pr_r464_2632_cpu pre_jl = {
    0x0C88, 0x2699, 0x0015, 0, 0x000D, 0, 0x2612,
    8, 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246
};

typedef struct {
    unsigned count;
    unsigned errors;
} stage_check;

static void stages(uint16_t ip, const uint8_t *ram,
                   const pr_r464_2632_cpu *cpu, void *context)
{
    static const uint16_t expected[] = {0x265A, 0x266E, 0x2679, 0x2699};
    stage_check *check = context;
    if (check->count >= 4u || ip != expected[check->count])
        ++check->errors;
    if (ip == 0x2679u &&
        (cpu->ax != 2u || cpu->flags != 0x7246u ||
         get_word(ram, cpu->ss, 0x3E72u) != 2u))
        ++check->errors;
    if (ip == 0x2699u &&
        (cpu->ax != 21u || cpu->cx != 13u ||
         cpu->bx != 0u || cpu->es != 0x3DE4u))
        ++check->errors;
    ++check->count;
}

static void setup(unsigned char *ram, pr_r464_2632_cpu *cpu)
{
    memset(ram, 0xA5, PR_R464_2632_RAM_BYTES);
    *cpu = positive_pre;
    put_word(ram, cpu->ss, 0x3E86u, 8u);
    put_word(ram, cpu->ss, 0x3E88u, 0x3DE4u);
    put_word(ram, cpu->ss, 0x3E92u, 11u);
    put_word(ram, cpu->ss, 0x3E96u, 2u);
    put_word(ram, 0x3DE4u, 0x000Au, 21u);
}

static void test_synthetic(void)
{
    unsigned char *ram = malloc(PR_R464_2632_RAM_BYTES);
    unsigned char *before = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu;
    stage_check check = {0, 0};
    assert(ram != NULL && before != NULL);
    setup(ram, &cpu);
    assert(pr_r464_2632_fourth_positive(
        NULL, PR_R464_2632_RAM_BYTES, &cpu, NULL, NULL) ==
        PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_positive(
        ram, PRIVATE_RAM_BYTES, &cpu, NULL, NULL) ==
        PR_R464_2632_INVALID_HOST);
    ++cpu.ip;
    memcpy(before, ram, PR_R464_2632_RAM_BYTES);
    assert(pr_r464_2632_fourth_positive(
        ram, PR_R464_2632_RAM_BYTES, &cpu, NULL, NULL) ==
        PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(ram, before, PR_R464_2632_RAM_BYTES) == 0);
    --cpu.ip;
    put_word(ram, cpu.ss, 0x3E96u, 0xFFFFu);
    memcpy(before, ram, PR_R464_2632_RAM_BYTES);
    assert(pr_r464_2632_fourth_positive(
        ram, PR_R464_2632_RAM_BYTES, &cpu, NULL, NULL) ==
        PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(ram, before, PR_R464_2632_RAM_BYTES) == 0);
    setup(ram, &cpu);
    put_word(ram, cpu.ss, 0x3E92u, 65u);
    memcpy(before, ram, PR_R464_2632_RAM_BYTES);
    assert(pr_r464_2632_fourth_positive(
        ram, PR_R464_2632_RAM_BYTES, &cpu, NULL, NULL) ==
        PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(ram, before, PR_R464_2632_RAM_BYTES) == 0);
    setup(ram, &cpu);
    assert(pr_r464_2632_fourth_positive(
        ram, PR_R464_2632_RAM_BYTES, &cpu, stages, &check) ==
        PR_R464_2632_OK);
    assert(check.count == 4u && check.errors == 0u);
    assert(memcmp(&cpu, &pre_jl, sizeof(cpu)) == 0);
    assert(get_word(ram, cpu.ss, 0x3E76u) == 0u);
    assert(get_word(ram, cpu.ss, 0x3E72u) == 2u);

    setup(ram, &cpu);
    put_word(ram, cpu.ss, 0x3E92u, 5u);
    put_word(ram, cpu.ss, 0x3E96u, 3u);
    put_word(ram, 0x3DE4u, 0x000Au, 25u);
    assert(pr_r464_2632_fourth_positive(
        ram, PR_R464_2632_RAM_BYTES, &cpu, NULL, NULL) ==
        PR_R464_2632_OK);
    assert(cpu.ip == 0x2699u && cpu.ax == 25u && cpu.cx == 8u);
    assert(cpu.flags == 0x7246u && get_word(ram, cpu.ss, 0x3E72u) == 3u);
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

static int test_replay(const char *pre_path, const char *next_path)
{
    unsigned char *ram = calloc(PR_R464_2632_RAM_BYTES, 1);
    unsigned char *next = malloc(PRIVATE_RAM_BYTES);
    pr_r464_2632_cpu cpu = positive_pre;
    unsigned differences = 0;
    int status;
    if (ram == NULL || next == NULL || !load_private(pre_path, ram) ||
        !load_private(next_path, next)) {
        fprintf(stderr, "private RAM load failed\n");
        free(ram); free(next);
        return 1;
    }
    status = pr_r464_2632_fourth_positive(
        ram, PR_R464_2632_RAM_BYTES, &cpu, NULL, NULL);
    for (unsigned i = 0; i < PRIVATE_RAM_BYTES; ++i)
        differences += ram[i] != next[i];
    printf("fourth +2656..+2699 replay RAM mismatches: %u\n", differences);
    free(ram); free(next);
    return status != PR_R464_2632_OK || differences != 0u ||
           memcmp(&cpu, &pre_jl, sizeof(cpu)) != 0;
}

int main(int argc, char **argv)
{
    test_synthetic();
    if (argc == 3) return test_replay(argv[1], argv[2]);
    return argc == 1 ? 0 : 2;
}
