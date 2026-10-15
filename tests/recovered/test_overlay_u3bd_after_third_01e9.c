#include "poolrad/recovered/overlay_u3bd_after_third_01e9.h"

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

static const pr_r464_2632_cpu parent_pre = {
    0x2D14, 0x01E9, 1, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x007A, 0x3E98, 0x3E98, 0x14A0, 0x3DE4, 0x1D16, 0x7246
};
static const pr_r464_2632_cpu fourth_call_pre = {
    0x2D14, 0x020A, 6, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x260A, 0x3E98, 0x3E86, 0x14A0, 0x3DE4, 0x1D16, 0x7246
};

static void setup(unsigned char *ram, pr_r464_2632_cpu *cpu)
{
    memset(ram, 0, PR_R464_2632_RAM_BYTES);
    *cpu = parent_pre;
    ram[address(cpu->ds, 0x6A63u)] = 6u;
    put_word(ram, cpu->ds, 0x4A20u, 8u);
    put_word(ram, cpu->ds, 0x4A22u, 0x3DE4u);
    for (unsigned i = 0; i < 9u; ++i)
        put_word(ram, cpu->ss, 0x3E86u + 2u * i, 0xA500u + i);
}

static void test_synthetic(void)
{
    unsigned char *ram = malloc(PR_R464_2632_RAM_BYTES);
    unsigned char *before = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu;
    static const unsigned pushed[] = {
        8, 0x3DE4, 0x260A, 0x14A0, 6, 0x002A, 0x000B, 0x003E, 2
    };
    assert(ram != NULL && before != NULL);
    setup(ram, &cpu);
    assert(pr_u3bd_after_third_01e9(NULL, PR_R464_2632_RAM_BYTES,
                                     &cpu) == PR_R464_2632_INVALID_HOST);
    assert(pr_u3bd_after_third_01e9(ram, PRIVATE_RAM_BYTES,
                                     &cpu) == PR_R464_2632_INVALID_HOST);
    ++cpu.ip;
    memcpy(before, ram, PR_R464_2632_RAM_BYTES);
    assert(pr_u3bd_after_third_01e9(ram, PR_R464_2632_RAM_BYTES,
                                     &cpu) == PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(ram, before, PR_R464_2632_RAM_BYTES) == 0);
    --cpu.ip;
    assert(pr_u3bd_after_third_01e9(ram, PR_R464_2632_RAM_BYTES,
                                     &cpu) == PR_R464_2632_OK);
    assert(memcmp(&cpu, &fourth_call_pre, sizeof(cpu)) == 0);
    for (unsigned i = 0; i < 9u; ++i)
        assert(get_word(ram, cpu.ss, 0x3E86u + 2u * i) == pushed[i]);
    setup(ram, &cpu);
    ram[address(cpu.ds, 0x6A63u)] = 0xA7u;
    assert(pr_u3bd_after_third_01e9(ram, PR_R464_2632_RAM_BYTES,
                                     &cpu) == PR_R464_2632_OK);
    assert(cpu.ax == 0x00A7u && cpu.flags == 0x7246u);
    assert(get_word(ram, cpu.ss, 0x3E8Eu) == 0x00A7u);
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
    pr_r464_2632_cpu cpu = parent_pre;
    unsigned differences = 0;
    int status;
    if (ram == NULL || next == NULL || !load_private(pre_path, ram) ||
        !load_private(next_path, next)) {
        fprintf(stderr, "private RAM load failed\n");
        free(ram); free(next);
        return 1;
    }
    status = pr_u3bd_after_third_01e9(ram, PR_R464_2632_RAM_BYTES, &cpu);
    for (unsigned i = 0; i < PRIVATE_RAM_BYTES; ++i)
        differences += ram[i] != next[i];
    printf("parent +01E9..+020A replay RAM mismatches: %u\n", differences);
    free(ram); free(next);
    return status != PR_R464_2632_OK || differences != 0u ||
           memcmp(&cpu, &fourth_call_pre, sizeof(cpu)) != 0;
}

int main(int argc, char **argv)
{
    test_synthetic();
    if (argc == 3) return test_replay(argv[1], argv[2]);
    return argc == 1 ? 0 : 2;
}
