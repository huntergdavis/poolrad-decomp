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

static pr_r464_2632_cpu entry_cpu(void)
{
    return (pr_r464_2632_cpu){
        0x0C88, 0x2632, 0x000B, 0, 4, 0x000F, 0x02DF, 0x260A,
        0x3E98, 0x3E82, 0x14A0, 0x3996, 0x1D16, 0x7246
    };
}

static void setup(unsigned char *ram, pr_r464_2632_cpu *cpu)
{
    const unsigned bp = 0x3E80u;
    memset(ram, 0, PR_R464_2632_RAM_BYTES);
    *cpu = entry_cpu();
    set_word(ram, cpu->ss, bp, 0x3E98u);
    set_word(ram, cpu->ss, bp + 2u, 0x019Du);
    set_word(ram, cpu->ss, bp + 4u, 0x2D14u);
    set_word(ram, cpu->ss, bp + 6u, 8u);
    set_word(ram, cpu->ss, bp + 8u, 0x3DE4u);
    set_word(ram, cpu->ss, bp + 0x0Au, 0x260Au);
    set_word(ram, cpu->ss, bp + 0x0Cu, 0x14A0u);
    set_word(ram, cpu->ss, bp + 0x0Eu, 0x000Bu);
    set_word(ram, cpu->ss, bp + 0x10u, 43u);
    set_word(ram, cpu->ss, bp + 0x12u, 11u);
    set_word(ram, cpu->ss, bp + 0x14u, 16u);
    set_word(ram, cpu->ss, bp + 0x16u, 2u);
    set_word(ram, 0x3DE4u, 8u, 168u);
    set_word(ram, 0x3DE4u, 10u, 21u);
    for (unsigned i = 0; i < 4u; ++i)
        ram[address(cpu->ds, 0x25F6u + i)] = (unsigned char)(1u << i);
    ram[address(cpu->ds, 0x4A1Eu)] = 1u;
    for (unsigned row = 16u; row < 59u; ++row)
        set_word(ram, cpu->ds, 0x4C20u + row * 2u, 0xFFFFu);
}

static void test_synthetic(void)
{
    unsigned char *ram = malloc(PR_R464_2632_RAM_BYTES);
    unsigned char *before = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu, old_cpu;
    static const unsigned char plane[4] = {0xFFu, 0xFFu, 0u, 0xFFu};
    assert(ram && before);
    setup(ram, &cpu);
    memcpy(before, ram, PR_R464_2632_RAM_BYTES);
    old_cpu = cpu;
    assert(pr_r464_2632_first_call(NULL, PR_R464_2632_RAM_BYTES,
                                    &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    set_word(ram, cpu.ss, 0x3E80u + 0x0Eu, 0x000Fu);
    assert(pr_r464_2632_first_call(ram, PR_R464_2632_RAM_BYTES,
                                    &cpu, NULL, NULL) ==
           PR_R464_2632_UNSUPPORTED_PATH);
    set_word(ram, cpu.ss, 0x3E80u + 0x0Eu, 0x000Bu);
    assert(memcmp(ram, before, PR_R464_2632_RAM_BYTES) == 0);
    assert(memcmp(&cpu, &old_cpu, sizeof(cpu)) == 0);
    assert(pr_r464_2632_first_call(ram, PR_R464_2632_RAM_BYTES,
                                    &cpu, NULL, NULL) == PR_R464_2632_OK);
    assert(cpu.cs == 0x2D14u && cpu.ip == 0x019Du);
    assert(cpu.sp == 0x3E98u && cpu.bp == 0x3E98u);
    assert(cpu.flags == 0x7246u);
    for (unsigned row = 16u; row < 59u; ++row) {
        unsigned start = 8u + 0x17u + (row * 21u + 2u) * 4u;
        assert(ram[address(cpu.ds, 0x4A28u + row)] == 1u);
        assert(word(ram, cpu.ds, 0x4AD0u + 2u * row) == 2u);
        assert(word(ram, cpu.ds, 0x4C20u + 2u * row) ==
               (row * 21u + 2u) * 4u);
        assert(word(ram, cpu.ds, 0x4D70u + 2u * row) ==
               (row * 21u + 2u + 11u) * 4u - 1u);
        for (unsigned x = 0; x < 11u; ++x)
            for (unsigned p = 0; p < 4u; ++p)
                assert(ram[address(0x3DE4u, start + x * 4u + p)] == plane[p]);
    }
    assert(ram[address(cpu.ds, 0x70C7u)] == 11u);
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
    const unsigned char *loop_exit;
    unsigned seen;
    unsigned errors;
} observer_context;

static void check_loop_exit(uint16_t ip, const uint8_t *ram,
                            const pr_r464_2632_cpu *cpu, void *context)
{
    observer_context *state = context;
    const pr_r464_2632_cpu expected_cpu = {
        0x0C88, 0x2A07, 0x002A, 0x3E4E, 0, 0x14A0, 0x3E68, 0x0074,
        0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246
    };
    if (ip != 0x2A07u) return;
    ++state->seen;
    if (memcmp(ram, state->loop_exit, PRIVATE_RAM_BYTES) != 0) {
        unsigned differences = 0;
        for (unsigned i = 0; i < PRIVATE_RAM_BYTES; ++i)
            differences += ram[i] != state->loop_exit[i];
        fprintf(stderr, "+2A07 RAM mismatches: %u\n", differences);
        ++state->errors;
    }
    if (memcmp(cpu, &expected_cpu, sizeof(*cpu)) != 0)
        ++state->errors;
}

static int test_native(const char *caller_pre, const char *loop_path,
                       const char *caller_return)
{
    unsigned char *ram = calloc(PR_R464_2632_RAM_BYTES, 1u);
    unsigned char *expected = malloc(PRIVATE_RAM_BYTES);
    unsigned char *loop_exit = malloc(PRIVATE_RAM_BYTES);
    pr_r464_2632_cpu cpu = entry_cpu();
    const pr_r464_2632_cpu expected_return_cpu = {
        0x2D14, 0x019D, 0x002A, 0x3E4E, 0, 0x14A0, 0x3E68, 0x0074,
        0x3E98, 0x3E98, 0x14A0, 0x3DE4, 0x1D16, 0x7246
    };
    observer_context state = {loop_exit, 0u, 0u};
    unsigned differences = 0;
    if (!ram || !expected || !loop_exit ||
        !load_private(caller_pre, ram) ||
        !load_private(loop_path, loop_exit) ||
        !load_private(caller_return, expected)) {
        fprintf(stderr, "private image load failed\n");
        free(ram); free(expected); free(loop_exit);
        return 1;
    }
    /* The original five-byte far CALL at 2D14:0198 pushes CS then +019D. */
    set_word(ram, cpu.ss, 0x3E84u, 0x2D14u);
    set_word(ram, cpu.ss, 0x3E82u, 0x019Du);
    if (pr_r464_2632_first_call(ram, PR_R464_2632_RAM_BYTES, &cpu,
                                 check_loop_exit, &state) != PR_R464_2632_OK) {
        fprintf(stderr, "native first-call shape unsupported\n");
        free(ram); free(expected); free(loop_exit);
        return 1;
    }
    for (unsigned i = 0; i < PRIVATE_RAM_BYTES; ++i) {
        if (ram[i] != expected[i]) {
            if (differences < 12u) fprintf(stderr, "return RAM mismatch %05X\n", i);
            ++differences;
        }
    }
    printf("first-call +2A07 checks: %u, errors: %u; return RAM mismatches: %u\n",
           state.seen, state.errors, differences);
    printf("C return CPU: %04X %04X %04X %04X %04X %04X %04X %04X "
           "%04X %04X %04X %04X %04X %04X\n",
           cpu.cs, cpu.ip, cpu.ax, cpu.bx, cpu.cx, cpu.dx, cpu.si, cpu.di,
           cpu.bp, cpu.sp, cpu.ds, cpu.es, cpu.ss, cpu.flags);
    free(ram); free(expected); free(loop_exit);
    return differences != 0u || state.seen != 1u || state.errors != 0u ||
           memcmp(&cpu, &expected_return_cpu, sizeof(cpu)) != 0;
}

int main(int argc, char **argv)
{
    test_synthetic();
    if (argc == 4) return test_native(argv[1], argv[2], argv[3]);
    return argc == 1 ? 0 : 2;
}
