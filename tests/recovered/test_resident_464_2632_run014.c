#include "poolrad/recovered/resident_464_2632_run014.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { PRIVATE_RAM_BYTES = 655360 };

static unsigned address(unsigned seg, unsigned off)
{
    return ((seg << 4) + off) & 0xFFFFFu;
}

static unsigned word(const unsigned char *ram, unsigned seg, unsigned off)
{
    return ram[address(seg, off)] | ((unsigned)ram[address(seg, off + 1u)] << 8);
}

static void set_word(unsigned char *ram, unsigned seg, unsigned off, unsigned value)
{
    ram[address(seg, off)] = (unsigned char)value;
    ram[address(seg, off + 1u)] = (unsigned char)(value >> 8);
}

static void setup(unsigned char *ram, pr_r464_2632_cpu *cpu)
{
    unsigned bp = 0x3E80u;
    memset(ram, 0, PR_R464_2632_RAM_BYTES);
    *cpu = (pr_r464_2632_cpu){
        0x0C88, 0x2632, 0x000F, 0x3E4E, 0, 0x14A0, 0x3E68, 0x260A,
        0x3E98, 0x3E82, 0x14A0, 0x3DE4, 0x1D16, 0x7246
    };
    set_word(ram, cpu->ss, bp, 0x3E98u);
    set_word(ram, cpu->ss, bp + 2u, 0x01C3u);
    set_word(ram, cpu->ss, bp + 4u, 0x2D14u);
    set_word(ram, cpu->ss, bp + 6u, 8u);
    set_word(ram, cpu->ss, bp + 8u, 0x3DE4u);
    set_word(ram, cpu->ss, bp + 0x0Au, 0x260Au);
    set_word(ram, cpu->ss, bp + 0x0Cu, 0x14A0u);
    set_word(ram, cpu->ss, bp + 0x0Eu, 0x0Fu);
    set_word(ram, cpu->ss, bp + 0x10u, 1u);
    set_word(ram, cpu->ss, bp + 0x12u, 11u);
    set_word(ram, cpu->ss, bp + 0x14u, 59u);
    set_word(ram, cpu->ss, bp + 0x16u, 2u);
    set_word(ram, 0x3DE4u, 8u, 168u);
    set_word(ram, 0x3DE4u, 10u, 21u);
    for (unsigned i = 0; i < 4; ++i)
        ram[address(cpu->ds, 0x25F6u + i)] = (unsigned char)(1u << i);
    ram[address(cpu->ds, 0x4A1Eu)] = 1u;
    set_word(ram, cpu->ds, 0x4C20u + 118u, 0xFFFFu);
}

static void test_synthetic(void)
{
    unsigned char *ram = malloc(PR_R464_2632_RAM_BYTES);
    unsigned char *before = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu, old_cpu;
    assert(ram && before);
    setup(ram, &cpu);
    memcpy(before, ram, PR_R464_2632_RAM_BYTES);
    old_cpu = cpu;
    assert(pr_r464_2632_run014(NULL, PR_R464_2632_RAM_BYTES, &cpu,
                                NULL, NULL) == PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_run014(ram, PR_R464_2632_RAM_BYTES - 1u,
                                &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    ram[address(cpu.ds, 0x4A1Eu)] = 0u;
    assert(pr_r464_2632_run014(ram, PR_R464_2632_RAM_BYTES, &cpu,
                                NULL, NULL) ==
           PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(&cpu, &old_cpu, sizeof(cpu)) == 0);
    ram[address(cpu.ds, 0x4A1Eu)] = 1u;
    assert(memcmp(ram, before, PR_R464_2632_RAM_BYTES) == 0);
    assert(pr_r464_2632_run014(ram, PR_R464_2632_RAM_BYTES, &cpu,
                                NULL, NULL) == PR_R464_2632_OK);
    assert(cpu.cs == 0x2D14u && cpu.ip == 0x01C3u);
    assert(cpu.sp == 0x3E98u && cpu.bp == 0x3E98u);
    assert(cpu.flags == 0x7246u);
    for (unsigned i = 0; i < 44u; ++i)
        assert(ram[address(0x3DE4u, 0x1383u + i)] == 0xFFu);
    assert(ram[address(cpu.ds, 0x4A28u + 59u)] == 1u);
    assert(word(ram, cpu.ds, 0x4AD0u + 118u) == 2u);
    assert(word(ram, cpu.ds, 0x4C20u + 118u) == 0x1364u);
    assert(word(ram, cpu.ds, 0x4D70u + 118u) == 0x138Fu);
    assert(ram[address(cpu.ds, 0x70C7u)] == 11u);
    setup(ram, &cpu);
    ram[address(cpu.ds, 0x25F6u)] = 0u;
    memcpy(before, ram, PR_R464_2632_RAM_BYTES);
    old_cpu = cpu;
    assert(pr_r464_2632_run014(ram, PR_R464_2632_RAM_BYTES, &cpu,
                                NULL, NULL) ==
           PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(ram, before, PR_R464_2632_RAM_BYTES) == 0);
    assert(memcmp(&cpu, &old_cpu, sizeof(cpu)) == 0);
    setup(ram, &cpu);
    set_word(ram, cpu.ss, 0x3E80u + 0x12u, 3u);
    assert(pr_r464_2632_run014(ram, PR_R464_2632_RAM_BYTES, &cpu,
                                NULL, NULL) == PR_R464_2632_OK);
    for (unsigned i = 0; i < 12u; ++i)
        assert(ram[address(0x3DE4u, 0x1383u + i)] == 0xFFu);
    assert(ram[address(0x3DE4u, 0x1383u + 12u)] == 0u);
    assert(word(ram, cpu.ds, 0x4D70u + 118u) == 0x136Fu);
    assert(ram[address(cpu.ds, 0x70C7u)] == 3u);
    free(before);
    free(ram);
}

static int load_private(const char *path, unsigned char *out)
{
    FILE *f = fopen(path, "rb");
    size_t n;
    if (!f) return 0;
    n = fread(out, 1, PRIVATE_RAM_BYTES, f);
    if (fgetc(f) != EOF) { fclose(f); return 0; }
    fclose(f);
    return n == PRIVATE_RAM_BYTES;
}

typedef struct {
    unsigned ip;
    const char *name;
    pr_r464_2632_cpu cpu;
} checkpoint;

static const checkpoint checkpoints[] = {
    {0x2632, "ram-2632-second-res-entry.bin",
     {0x0C88,0x2632,0x000F,0x3E4E,0,0x14A0,0x3E68,0x260A,0x3E98,0x3E82,0x14A0,0x3DE4,0x1D16,0x7246}},
    {0x2646, "ram-2646-second-res-runtime-pre.bin",
     {0x0C88,0x2646,0x0008,0x3E4E,0,0x14A0,0x3E68,0x3E78,0x3E80,0x3E52,0x14A0,0x14A0,0x1D16,0x7216}},
    {0x264B, "ram-264b-second-res-runtime-post.bin",
     {0x0C88,0x264B,0x0008,0x3E4E,0,0x14A0,0x2612,0x3E80,0x3E80,0x3E5C,0x14A0,0x1D16,0x1D16,0x7216}},
    {0x28E0, "ram-28e0-second-res-je-target.bin",
     {0x0C88,0x28E0,0x0001,0,0x003C,0,0x2612,0x0008,0x3E80,0x3E5C,0x14A0,0x3DE4,0x1D16,0x7246}},
    {0x291C, "ram-291c-second-res-call-pre.bin",
     {0x0C88,0x291C,0,0,0x003C,0,0x2612,0x3E64,0x3E80,0x3E54,0x14A0,0x3DE4,0x1D16,0x7216}},
    {0x2921, "ram-2921-second-res-fill-return.bin",
     {0x0C88,0x2921,0,0x3E50,0,0,0x2612,0x3E68,0x3E80,0x3E5C,0x14A0,0x1D16,0x1D16,0x7216}},
    {0x294D, "ram-294d-second-res-mask-loop-exit.bin",
     {0x0C88,0x294D,3,0x3E50,0,0,0x2612,3,0x3E80,0x3E5C,0x14A0,0x1D16,0x1D16,0x7246}},
    {0x2999, "ram-2999-second-res-limit-stores.bin",
     {0x0C88,0x2999,0x1364,0x3E50,0,0,0x2612,0x0076,0x3E80,0x3E5C,0x14A0,0x1D16,0x1D16,0x7212}},
    {0x29C6, "ram-29c6-second-res-first-move-pre.bin",
     {0x0C88,0x29C6,4,0x3E50,0,0,0x2612,0x1383,0x3E80,0x3E52,0x14A0,0x3DE4,0x1D16,0x7212}},
    {0x29CB, "ram-29cb-second-res-first-move-return.bin",
     {0x0C88,0x29CB,4,0x3E4E,0,0x14A0,0x3E68,0x1387,0x3E80,0x3E5C,0x14A0,0x3DE4,0x1D16,0x7206}},
    {0x29D7, "ram-29d7-second-res-move-loop-exit.bin",
     {0x0C88,0x29D7,0x000B,0x3E4E,0,0x14A0,0x3E68,0x13AF,0x3E80,0x3E5C,0x14A0,0x3DE4,0x1D16,0x7246}},
    {0x2A02, "ram-2a02-second-res-final-je-pre.bin",
     {0x0C88,0x2A02,0,0x3E4E,0,0x14A0,0x3E68,0x0076,0x3E80,0x3E5C,0x14A0,0x3DE4,0x1D16,0x7246}},
    {0x2A07, "ram-2a07-second-res-epilogue-pre.bin",
     {0x0C88,0x2A07,0,0x3E4E,0,0x14A0,0x3E68,0x0076,0x3E80,0x3E5C,0x14A0,0x3DE4,0x1D16,0x7246}},
    {0x01C3, "ram-01c3-second-res-caller-return.bin",
     {0x2D14,0x01C3,0,0x3E4E,0,0x14A0,0x3E68,0x0076,0x3E98,0x3E98,0x14A0,0x3DE4,0x1D16,0x7246}}
};

typedef struct {
    const char *directory;
    unsigned char *expected;
    unsigned seen;
    unsigned errors;
} native_observer;

static void compare_checkpoint(uint16_t ip, const unsigned char *ram,
                               const pr_r464_2632_cpu *cpu, void *context)
{
    native_observer *state = context;
    const checkpoint *check = NULL;
    char path[1024];
    unsigned differences = 0;
    for (unsigned j = 0; j < sizeof(checkpoints)/sizeof(checkpoints[0]); ++j)
        if (checkpoints[j].ip == ip) { check = &checkpoints[j]; break; }
    if (!check) { fprintf(stderr, "unexpected checkpoint %04X\n", ip); ++state->errors; return; }
    if (snprintf(path, sizeof(path), "%s/%s", state->directory, check->name) >=
        (int)sizeof(path) || !load_private(path, state->expected)) {
        fprintf(stderr, "checkpoint load failed: %04X\n", ip);
        ++state->errors;
        return;
    }
    for (unsigned j = 0; j < PRIVATE_RAM_BYTES; ++j) {
        if (ram[j] != state->expected[j]) {
            if (differences < 4u)
                fprintf(stderr, "%04X RAM mismatch at %05X\n", ip, j);
            ++differences;
        }
    }
    if (differences != 0u) {
        fprintf(stderr, "%04X RAM mismatches: %u\n", ip, differences);
        ++state->errors;
    }
    if (memcmp(cpu, &check->cpu, sizeof(*cpu)) != 0) {
        const uint16_t *a = (const uint16_t *)cpu;
        const uint16_t *b = (const uint16_t *)&check->cpu;
        for (unsigned j = 0; j < sizeof(*cpu)/sizeof(uint16_t); ++j)
            if (a[j] != b[j])
                fprintf(stderr, "%04X CPU field %u: %04X != %04X\n",
                        ip, j, a[j], b[j]);
        ++state->errors;
    }
    ++state->seen;
}

static int test_native(const char *entry_path, const char *return_path)
{
    unsigned char *ram = calloc(PR_R464_2632_RAM_BYTES, 1u);
    unsigned char *expected = malloc(PRIVATE_RAM_BYTES);
    unsigned errors = 0;
    pr_r464_2632_cpu cpu = {
        0x0C88, 0x2632, 0x000F, 0x3E4E, 0, 0x14A0, 0x3E68, 0x260A,
        0x3E98, 0x3E82, 0x14A0, 0x3DE4, 0x1D16, 0x7246
    };
    const pr_r464_2632_cpu final = {
        0x2D14, 0x01C3, 0, 0x3E4E, 0, 0x14A0, 0x3E68, 0x0076,
        0x3E98, 0x3E98, 0x14A0, 0x3DE4, 0x1D16, 0x7246
    };
    char directory[1024];
    const char *slash = strrchr(entry_path, '/');
    native_observer state = {directory, expected, 0, 0};
    if (!slash || (size_t)(slash - entry_path) >= sizeof(directory)) {
        free(ram);
        free(expected);
        return 1;
    }
    memcpy(directory, entry_path, (size_t)(slash - entry_path));
    directory[slash - entry_path] = '\0';
    if (!ram || !expected || !load_private(entry_path, ram) ||
        !load_private(return_path, expected)) {
        fprintf(stderr, "private RAM load failed\n");
        free(ram);
        free(expected);
        return 1;
    }
    if (pr_r464_2632_run014(ram, PR_R464_2632_RAM_BYTES,
                             &cpu, compare_checkpoint, &state) != PR_R464_2632_OK) {
        fprintf(stderr, "private path unsupported\n");
        free(ram);
        free(expected);
        return 1;
    }
    for (unsigned i = 0; i < PRIVATE_RAM_BYTES; ++i) {
        if (ram[i] != expected[i]) {
            if (errors < 24u) fprintf(stderr, "RAM mismatch at %05X\n", i);
            ++errors;
        }
    }
    printf("private final RAM mismatches: %u\n", errors);
    printf("private checkpoints compared: %u/%zu; failures: %u\n",
           state.seen, sizeof(checkpoints)/sizeof(checkpoints[0]), state.errors);
    if (state.seen != sizeof(checkpoints)/sizeof(checkpoints[0]))
        ++errors;
    errors += state.errors;
    if (memcmp(&cpu, &final, sizeof(cpu)) != 0) {
        const uint16_t *a = (const uint16_t *)&cpu;
        const uint16_t *b = (const uint16_t *)&final;
        for (unsigned i = 0; i < sizeof(cpu) / sizeof(uint16_t); ++i)
            if (a[i] != b[i]) fprintf(stderr, "CPU field %u: %04X != %04X\n",
                                      i, a[i], b[i]);
        ++errors;
    }
    free(ram);
    free(expected);
    return errors != 0u;
}

int main(int argc, char **argv)
{
    test_synthetic();
    if (argc == 3) return test_native(argv[1], argv[2]);
    return argc == 1 ? 0 : 2;
}
