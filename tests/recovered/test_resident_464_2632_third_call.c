#include "poolrad/recovered/resident_464_2632_third_call.h"

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

static const pr_r464_2632_cpu caller_pre = {
    0x2D14, 0x01E4, 7, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x260A, 0x3E98, 0x3E86, 0x14A0, 0x3DE4, 0x1D16, 0x7246
};
static const pr_r464_2632_cpu resident_entry = {
    0x0C88, 0x2632, 7, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x260A, 0x3E98, 0x3E82, 0x14A0, 0x3DE4, 0x1D16, 0x7246
};
static const pr_r464_2632_cpu caller_return = {
    0x2D14, 0x01E9, 1, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x007A, 0x3E98, 0x3E98, 0x14A0, 0x3DE4, 0x1D16, 0x7246
};

static void setup(unsigned char *ram, pr_r464_2632_cpu *cpu)
{
    memset(ram, 0, PR_R464_2632_RAM_BYTES);
    *cpu = caller_pre;
    put_word(ram, cpu->ss, 0x3E82, 0xA55A);
    put_word(ram, cpu->ss, 0x3E84, 0x5AA5);
    put_word(ram, cpu->ss, 0x3E86, 8);
    put_word(ram, cpu->ss, 0x3E88, 0x3DE4);
    put_word(ram, cpu->ss, 0x3E8A, 0x260A);
    put_word(ram, cpu->ss, 0x3E8C, 0x14A0);
    put_word(ram, cpu->ss, 0x3E8E, 7);
    put_word(ram, cpu->ss, 0x3E90, 2);
    put_word(ram, cpu->ss, 0x3E92, 11);
    put_word(ram, cpu->ss, 0x3E94, 60);
    put_word(ram, cpu->ss, 0x3E96, 2);
    put_word(ram, 0x3DE4, 8, 168);
    put_word(ram, 0x3DE4, 10, 21);
    for (unsigned i = 0; i < 4; ++i)
        ram[address(cpu->ds, 0x25F6u + i)] = (unsigned char)(1u << i);
    ram[address(cpu->ds, 0x4A1E)] = 1;
    put_word(ram, cpu->ds, 0x4C20 + 120, 0xFFFF);
    put_word(ram, cpu->ds, 0x4C20 + 122, 0xFFFF);
}

typedef struct {
    const unsigned char *expected;
    unsigned seen;
    unsigned errors;
} entry_check;

static void check_entry(uint16_t ip, const uint8_t *ram,
                        const pr_r464_2632_cpu *cpu, void *context)
{
    entry_check *state = context;
    if (ip != 0x2632u) return;
    ++state->seen;
    if (memcmp(cpu, &resident_entry, sizeof(*cpu)) != 0) ++state->errors;
    if (state->expected != NULL &&
        memcmp(ram, state->expected, PRIVATE_RAM_BYTES) != 0)
        ++state->errors;
}

static void test_synthetic(void)
{
    unsigned char *ram = malloc(PR_R464_2632_RAM_BYTES);
    unsigned char *before = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu;
    entry_check state = {NULL, 0, 0};
    assert(ram != NULL && before != NULL);
    setup(ram, &cpu);
    assert(pr_r464_2632_third_call(NULL, PR_R464_2632_RAM_BYTES, &cpu,
                                    NULL, NULL) == PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_third_call(ram, PRIVATE_RAM_BYTES, &cpu,
                                    NULL, NULL) == PR_R464_2632_INVALID_HOST);
    ++cpu.ip;
    memcpy(before, ram, PR_R464_2632_RAM_BYTES);
    assert(pr_r464_2632_third_call(ram, PR_R464_2632_RAM_BYTES, &cpu,
                                    NULL, NULL) == PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(before, ram, PR_R464_2632_RAM_BYTES) == 0);
    --cpu.ip;
    ram[address(cpu.ds, 0x25F6)] = 0;
    memcpy(before, ram, PR_R464_2632_RAM_BYTES);
    assert(pr_r464_2632_third_call(ram, PR_R464_2632_RAM_BYTES, &cpu,
                                    NULL, NULL) == PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(&cpu, &caller_pre, sizeof(cpu)) == 0);
    assert(memcmp(before, ram, PR_R464_2632_RAM_BYTES) == 0);
    setup(ram, &cpu);
    assert(pr_r464_2632_third_call(ram, PR_R464_2632_RAM_BYTES, &cpu,
                                    check_entry, &state) == PR_R464_2632_OK);
    assert(state.seen == 1u && state.errors == 0u);
    assert(memcmp(&cpu, &caller_return, sizeof(cpu)) == 0);
    assert(get_word(ram, cpu.ss, 0x3E82) == 0x01E9u);
    assert(get_word(ram, cpu.ss, 0x3E84) == 0x2D14u);
    for (unsigned row = 60; row <= 61; ++row) {
        unsigned start = (row * 21u + 2u) * 4u;
        unsigned object = 8u + 0x17u + start;
        for (unsigned i = 0; i < 11u; ++i) {
            for (unsigned plane = 0; plane < 4u; ++plane)
                assert(ram[address(0x3DE4, object + i * 4u + plane)] ==
                       (plane < 3u ? 0xFFu : 0u));
        }
        assert(ram[address(cpu.ds, 0x4A28u + row)] == 1u);
        assert(get_word(ram, cpu.ds, 0x4AD0u + 2u * row) == 2u);
        assert(get_word(ram, cpu.ds, 0x4C20u + 2u * row) == start);
        assert(get_word(ram, cpu.ds, 0x4D70u + 2u * row) == start + 43u);
    }
    assert(ram[address(cpu.ds, 0x70C7)] == 11u);
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

static int test_replay(const char *pre_path, const char *entry_path,
                       const char *return_path)
{
    unsigned char *ram = calloc(PR_R464_2632_RAM_BYTES, 1);
    unsigned char *entry = malloc(PRIVATE_RAM_BYTES);
    unsigned char *returned = malloc(PRIVATE_RAM_BYTES);
    pr_r464_2632_cpu cpu = caller_pre;
    entry_check state = {entry, 0, 0};
    unsigned differences = 0;
    int status;
    if (ram == NULL || entry == NULL || returned == NULL ||
        !load_private(pre_path, ram) || !load_private(entry_path, entry) ||
        !load_private(return_path, returned)) {
        fprintf(stderr, "private RAM load failed\n");
        free(ram); free(entry); free(returned);
        return 1;
    }
    status = pr_r464_2632_third_call(ram, PR_R464_2632_RAM_BYTES, &cpu,
                                      check_entry, &state);
    for (unsigned i = 0; i < PRIVATE_RAM_BYTES; ++i)
        differences += ram[i] != returned[i];
    printf("third-call entry: %u checks, %u errors; replay return RAM "
           "mismatches: %u\n", state.seen, state.errors, differences);
    free(ram); free(entry); free(returned);
    return status != PR_R464_2632_OK || state.seen != 1u ||
           state.errors != 0u || differences != 0u ||
           memcmp(&cpu, &caller_return, sizeof(cpu)) != 0;
}

int main(int argc, char **argv)
{
    test_synthetic();
    if (argc == 4) return test_replay(argv[1], argv[2], argv[3]);
    return argc == 1 ? 0 : 2;
}
