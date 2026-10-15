#include "poolrad/recovered/resident_464_2632_fourth_prefix.h"

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

static const pr_r464_2632_cpu fourth_pre = {
    0x2D14, 0x020A, 6, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x260A, 0x3E98, 0x3E86, 0x14A0, 0x3DE4, 0x1D16, 0x7246
};
static const pr_r464_2632_cpu pre_move = {
    0x0C88, 0x2646, 8, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x3E78, 0x3E80, 0x3E52, 0x14A0, 0x14A0, 0x1D16, 0x7216
};

static void setup(unsigned char *ram, pr_r464_2632_cpu *cpu)
{
    memset(ram, 0xA5, PR_R464_2632_RAM_BYTES);
    *cpu = fourth_pre;
    put_word(ram, cpu->ss, 0x3E8Au, 0x260Au);
    put_word(ram, cpu->ss, 0x3E8Cu, 0x14A0u);
}

static void test_synthetic(void)
{
    unsigned char *ram = malloc(PR_R464_2632_RAM_BYTES);
    unsigned char *before = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu;
    assert(ram != NULL && before != NULL);
    setup(ram, &cpu);
    assert(pr_r464_2632_fourth_prefix(NULL, PR_R464_2632_RAM_BYTES,
                                       &cpu) == PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_prefix(ram, PRIVATE_RAM_BYTES,
                                       &cpu) == PR_R464_2632_INVALID_HOST);
    ++cpu.ip;
    memcpy(before, ram, PR_R464_2632_RAM_BYTES);
    assert(pr_r464_2632_fourth_prefix(ram, PR_R464_2632_RAM_BYTES,
                                       &cpu) == PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(ram, before, PR_R464_2632_RAM_BYTES) == 0);
    --cpu.ip;
    assert(pr_r464_2632_fourth_prefix(ram, PR_R464_2632_RAM_BYTES,
                                       &cpu) == PR_R464_2632_OK);
    assert(memcmp(&cpu, &pre_move, sizeof(cpu)) == 0);
    assert(get_word(ram, cpu.ss, 0x3E82u) == 0x020Fu);
    assert(get_word(ram, cpu.ss, 0x3E84u) == 0x2D14u);
    assert(get_word(ram, cpu.ss, 0x3E80u) == 0x3E98u);
    assert(get_word(ram, cpu.ss, 0x3E5Au) == 0x14A0u);
    assert(get_word(ram, cpu.ss, 0x3E58u) == 0x260Au);
    assert(get_word(ram, cpu.ss, 0x3E56u) == 0x1D16u);
    assert(get_word(ram, cpu.ss, 0x3E54u) == 0x3E78u);
    assert(get_word(ram, cpu.ss, 0x3E52u) == 8u);

    setup(ram, &cpu);
    put_word(ram, cpu.ss, 0x3E8Au, 0x1234u);
    put_word(ram, cpu.ss, 0x3E8Cu, 0x5678u);
    assert(pr_r464_2632_fourth_prefix(ram, PR_R464_2632_RAM_BYTES,
                                       &cpu) == PR_R464_2632_OK);
    assert(cpu.di == 0x3E78u && cpu.es == 0x5678u);
    assert(get_word(ram, cpu.ss, 0x3E58u) == 0x1234u);
    assert(get_word(ram, cpu.ss, 0x3E5Au) == 0x5678u);
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
    pr_r464_2632_cpu cpu = fourth_pre;
    unsigned differences = 0;
    int status;
    if (ram == NULL || next == NULL || !load_private(pre_path, ram) ||
        !load_private(next_path, next)) {
        fprintf(stderr, "private RAM load failed\n");
        free(ram); free(next);
        return 1;
    }
    status = pr_r464_2632_fourth_prefix(
        ram, PR_R464_2632_RAM_BYTES, &cpu);
    for (unsigned i = 0; i < PRIVATE_RAM_BYTES; ++i)
        differences += ram[i] != next[i];
    printf("fourth +020A..+2646 replay RAM mismatches: %u\n", differences);
    free(ram); free(next);
    return status != PR_R464_2632_OK || differences != 0u ||
           memcmp(&cpu, &pre_move, sizeof(cpu)) != 0;
}

int main(int argc, char **argv)
{
    test_synthetic();
    if (argc == 3) return test_replay(argv[1], argv[2]);
    return argc == 1 ? 0 : 2;
}
