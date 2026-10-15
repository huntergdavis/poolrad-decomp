#include "poolrad/recovered/resident_464_2632_fourth_move.h"

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

static const pr_r464_2632_cpu move_pre = {
    0x0C88, 0x2646, 8, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x3E78, 0x3E80, 0x3E52, 0x14A0, 0x14A0, 0x1D16, 0x7216
};
static const pr_r464_2632_cpu successor = {
    0x0C88, 0x2656, 0x3DEC, 0x3E4E, 0, 0x14A0, 0x2612,
    0x3E80, 0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7202
};

typedef struct {
    unsigned count;
    unsigned errors;
} stage_check;

static void stages(uint16_t ip, const uint8_t *ram,
                   const pr_r464_2632_cpu *cpu, void *context)
{
    static const uint16_t expected[] = {0x0A3F, 0x264B, 0x2651, 0x2656};
    stage_check *check = context;
    if (check->count >= 4u || ip != expected[check->count])
        ++check->errors;
    if (ip == 0x0A3Fu) {
        if (cpu->cs != 0x131Cu || cpu->sp != 0x3E4Eu ||
            get_word(ram, cpu->ss, cpu->sp) != 0x264Bu ||
            get_word(ram, cpu->ss, cpu->sp + 2u) != 0x0C88u)
            ++check->errors;
    } else if (ip == 0x264Bu) {
        if (cpu->cs != 0x0C88u || cpu->sp != 0x3E5Cu ||
            cpu->si != 0x2612u || cpu->di != 0x3E80u)
            ++check->errors;
    }
    ++check->count;
}

static void setup(unsigned char *ram, pr_r464_2632_cpu *cpu)
{
    memset(ram, 0xA5, PR_R464_2632_RAM_BYTES);
    *cpu = move_pre;
    put_word(ram, cpu->ss, 0x3E52u, 8u);
    put_word(ram, cpu->ss, 0x3E54u, 0x3E78u);
    put_word(ram, cpu->ss, 0x3E56u, cpu->ss);
    put_word(ram, cpu->ss, 0x3E58u, 0x260Au);
    put_word(ram, cpu->ss, 0x3E5Au, cpu->ds);
    put_word(ram, cpu->ss, 0x3E86u, 8u);
    put_word(ram, cpu->ss, 0x3E88u, 0x3DE4u);
    for (unsigned i = 0; i < 8u; ++i) {
        ram[address(cpu->ds, 0x260Au + i)] = (unsigned char)(0x30u + i);
        ram[address(cpu->ss, 0x3E78u + i)] = 0xEEu;
    }
}

static void test_synthetic(void)
{
    unsigned char *ram = malloc(PR_R464_2632_RAM_BYTES);
    unsigned char *before = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu;
    stage_check check = {0, 0};
    assert(ram != NULL && before != NULL);
    setup(ram, &cpu);
    assert(pr_r464_2632_fourth_move(
        NULL, PR_R464_2632_RAM_BYTES, &cpu, NULL, NULL) ==
        PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_move(
        ram, PRIVATE_RAM_BYTES, &cpu, NULL, NULL) ==
        PR_R464_2632_INVALID_HOST);
    ++cpu.ip;
    memcpy(before, ram, PR_R464_2632_RAM_BYTES);
    assert(pr_r464_2632_fourth_move(
        ram, PR_R464_2632_RAM_BYTES, &cpu, NULL, NULL) ==
        PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(ram, before, PR_R464_2632_RAM_BYTES) == 0);
    --cpu.ip;
    put_word(ram, cpu.ss, 0x3E86u, 0u);
    put_word(ram, cpu.ss, 0x3E88u, 0u);
    memcpy(before, ram, PR_R464_2632_RAM_BYTES);
    assert(pr_r464_2632_fourth_move(
        ram, PR_R464_2632_RAM_BYTES, &cpu, NULL, NULL) ==
        PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(ram, before, PR_R464_2632_RAM_BYTES) == 0);
    setup(ram, &cpu);
    assert(pr_r464_2632_fourth_move(
        ram, PR_R464_2632_RAM_BYTES, &cpu, stages, &check) ==
        PR_R464_2632_OK);
    assert(check.count == 4u && check.errors == 0u);
    assert(memcmp(&cpu, &successor, sizeof(cpu)) == 0);
    assert(get_word(ram, cpu.ss, 0x3E4Eu) == 0x264Bu);
    for (unsigned i = 0; i < 8u; ++i)
        assert(ram[address(cpu.ss, 0x3E78u + i)] == 0x30u + i);
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
    pr_r464_2632_cpu cpu = move_pre;
    unsigned differences = 0;
    int status;
    if (ram == NULL || next == NULL || !load_private(pre_path, ram) ||
        !load_private(next_path, next)) {
        fprintf(stderr, "private RAM load failed\n");
        free(ram); free(next);
        return 1;
    }
    status = pr_r464_2632_fourth_move(
        ram, PR_R464_2632_RAM_BYTES, &cpu, NULL, NULL);
    for (unsigned i = 0; i < PRIVATE_RAM_BYTES; ++i)
        differences += ram[i] != next[i];
    printf("fourth +2646..+2656 replay RAM mismatches: %u\n", differences);
    free(ram); free(next);
    return status != PR_R464_2632_OK || differences != 0u ||
           memcmp(&cpu, &successor, sizeof(cpu)) != 0;
}

int main(int argc, char **argv)
{
    test_synthetic();
    if (argc == 3) return test_replay(argv[1], argv[2]);
    return argc == 1 ? 0 : 2;
}
