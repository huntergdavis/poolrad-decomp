#include "poolrad/recovered/resident_464_2632_fourth_2981.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    STACK_BASE = 0x1D16u << 4,
    GLOBAL_FIRST = (0x14A0u << 4) + 0x4B4Cu,
    GLOBAL_SECOND = (0x14A0u << 4) + 0x4C9Cu,
    LOCAL_FIRST = STACK_BASE + 0x3E60u,
    LOCAL_INDEX = STACK_BASE + 0x3E62u,
    LOCAL_SECOND = STACK_BASE + 0x3E6Au,
    LOCAL_SOURCE = STACK_BASE + 0x3E6Eu,
    LOCAL_COPY = STACK_BASE + 0x3E5Cu
};

static const pr_r464_2632_cpu pre = {
    0x0C88, 0x2981, 0xFFFF, 0x3E50, 0, 0, 0x2612, 0x007C,
    0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7286
};
static const pr_r464_2632_cpu saved_post = {
    0x0C88, 0x29A6, 0x1401, 0x3E50, 0, 0, 0x2612, 0x007C,
    0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7297
};
static const uint16_t prefix_ips[13] = {
    0x2981, 0x2984, 0x2987, 0x2989, 0x298D,
    0x2990, 0x2993, 0x2995, 0x2999, 0x299C,
    0x299F, 0x29A1, 0x29A4
};

static void put_word(uint8_t *ram, size_t address, uint16_t value)
{
    ram[address] = (uint8_t)value;
    ram[address + 1u] = (uint8_t)(value >> 8);
}

static void seed(uint8_t *ram, uint8_t source)
{
    memset(ram, 0xA5, PR_R464_2632_RAM_BYTES);
    put_word(ram, LOCAL_FIRST, 2u);
    put_word(ram, LOCAL_INDEX, 0x003Eu);
    put_word(ram, LOCAL_SECOND, 0x1460u);
    ram[LOCAL_SOURCE] = source;
    ram[LOCAL_COPY] = 0x7Au;
    put_word(ram, GLOBAL_FIRST, 0xFFFFu);
    put_word(ram, GLOBAL_SECOND, 0xFFFFu);
}

typedef struct { unsigned count, errors; uint16_t last; } observation;

static void observe(uint16_t ip, const uint8_t *ram,
                    const pr_r464_2632_cpu *cpu, void *context)
{
    observation *seen = context;
    (void)ram;
    if (cpu->cs != 0x0C88u || cpu->ip != ip || seen->count >= 14u ||
        (seen->count < 13u && ip != prefix_ips[seen->count]))
        ++seen->errors;
    seen->last = ip;
    ++seen->count;
}

static void test_saved_fallthrough(void)
{
    uint8_t *ram = malloc(PR_R464_2632_RAM_BYTES);
    uint8_t *expected = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu = pre;
    observation seen = {0u, 0u, 0u};
    assert(ram != NULL && expected != NULL);
    seed(ram, 0x0Bu);
    memcpy(expected, ram, PR_R464_2632_RAM_BYTES);
    put_word(expected, GLOBAL_FIRST, 2u);
    put_word(expected, GLOBAL_SECOND, 0x1460u);
    expected[LOCAL_COPY] = 0x0Bu;
    assert(pr_r464_2632_fourth_2981(
               ram, PR_R464_2632_RAM_BYTES, &cpu, observe, &seen) ==
           PR_R464_2632_OK);
    assert(memcmp(ram, expected, PR_R464_2632_RAM_BYTES) == 0);
    assert(memcmp(&cpu, &saved_post, sizeof(cpu)) == 0);
    assert(seen.count == 14u && seen.errors == 0u && seen.last == 0x29A6u);
    free(expected); free(ram);
}

static void test_greater_takes_branch(void)
{
    uint8_t *ram = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu = pre;
    observation seen = {0u, 0u, 0u};
    assert(ram != NULL);
    seed(ram, 0u);
    assert(pr_r464_2632_fourth_2981(
               ram, PR_R464_2632_RAM_BYTES, &cpu, observe, &seen) ==
           PR_R464_2632_OK);
    assert(cpu.cs == 0x0C88u && cpu.ip == 0x29D7u &&
           cpu.ax == 0x1401u && cpu.di == 0x007Cu &&
           cpu.flags == 0x7202u);
    assert(ram[GLOBAL_FIRST] == 2u && ram[GLOBAL_SECOND] == 0x60u &&
           ram[LOCAL_COPY] == 0u);
    assert(seen.count == 14u && seen.errors == 0u && seen.last == 0x29D7u);
    free(ram);
}

static void test_invalid_and_unsupported(void)
{
    uint8_t *ram = malloc(PR_R464_2632_RAM_BYTES);
    uint8_t *copy = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu = pre, old;
    assert(ram != NULL && copy != NULL);
    seed(ram, 0x0Bu);
    memcpy(copy, ram, PR_R464_2632_RAM_BYTES);
    assert(pr_r464_2632_fourth_2981(
               NULL, PR_R464_2632_RAM_BYTES, &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_2981(
               ram, 655360u, &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_2981(
               ram, PR_R464_2632_RAM_BYTES, NULL, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    cpu.ip = 0x2982u;
    old = cpu;
    assert(pr_r464_2632_fourth_2981(
               ram, PR_R464_2632_RAM_BYTES, &cpu, NULL, NULL) ==
           PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(&cpu, &old, sizeof(cpu)) == 0);
    assert(memcmp(ram, copy, PR_R464_2632_RAM_BYTES) == 0);
    free(copy); free(ram);
}

static int load_private(const char *path, uint8_t *out)
{
    FILE *f = fopen(path, "rb");
    size_t n;
    if (f == NULL) return 0;
    n = fread(out, 1, 655360u, f);
    if (fgetc(f) != EOF) { fclose(f); return 0; }
    fclose(f);
    return n == 655360u;
}

typedef struct {
    const char *directory;
    FILE *manifest;
    uint8_t *expected_ram;
    unsigned count, errors;
} private_check;

static void private_stage(uint16_t ip, const uint8_t *ram,
                          const pr_r464_2632_cpu *cpu, void *context)
{
    private_check *check = context;
    const uint16_t actual[14] = {
        cpu->cs, cpu->ip, cpu->ax, cpu->bx, cpu->cx, cpu->dx,
        cpu->si, cpu->di, cpu->bp, cpu->sp, cpu->ds, cpu->es,
        cpu->ss, cpu->flags
    };
    char line[256], path[1024], *cursor;
    unsigned i;
    int n;
    if (check->count >= 14u || fgets(line, sizeof(line), check->manifest) == NULL) {
        ++check->errors; ++check->count; return;
    }
    cursor = line;
    for (i = 0u; i < 14u; ++i) {
        char *end;
        unsigned long value = strtoul(cursor, &end, 16);
        if (end == cursor || value > 0xFFFFu || actual[i] != value)
            ++check->errors;
        cursor = end;
    }
    n = snprintf(path, sizeof(path), "%s/ram-%02u.bin",
                 check->directory, check->count);
    if (ip != cpu->ip || n < 0 || (size_t)n >= sizeof(path) ||
        !load_private(path, check->expected_ram) ||
        memcmp(ram, check->expected_ram, 655360u) != 0)
        ++check->errors;
    ++check->count;
}

static int test_replay(const char *pre_path, const char *directory)
{
    uint8_t *ram = calloc(PR_R464_2632_RAM_BYTES, 1);
    uint8_t *expected = malloc(655360u);
    char manifest_path[1024];
    private_check check;
    pr_r464_2632_cpu cpu = pre;
    pr_r464_2632_status status;
    int n, failed;
    if (ram == NULL || expected == NULL || !load_private(pre_path, ram)) {
        free(ram); free(expected); return 1;
    }
    n = snprintf(manifest_path, sizeof(manifest_path),
                 "%s/cpu-stops.txt", directory);
    if (n < 0 || (size_t)n >= sizeof(manifest_path)) {
        free(ram); free(expected); return 1;
    }
    check.directory = directory;
    check.manifest = fopen(manifest_path, "r");
    check.expected_ram = expected;
    check.count = 0u;
    check.errors = 0u;
    if (check.manifest == NULL) {
        free(ram); free(expected); return 1;
    }
    status = pr_r464_2632_fourth_2981(
        ram, PR_R464_2632_RAM_BYTES, &cpu, private_stage, &check);
    failed = status != PR_R464_2632_OK || check.count != 14u ||
             check.errors != 0u || memcmp(&cpu, &saved_post, sizeof(cpu)) != 0;
    printf("fourth +2981 checkpoints: %u errors: %u\n",
           check.count, check.errors);
    fclose(check.manifest);
    free(expected); free(ram);
    return failed;
}

int main(int argc, char **argv)
{
    test_saved_fallthrough();
    test_greater_takes_branch();
    test_invalid_and_unsupported();
    if (argc == 3) return test_replay(argv[1], argv[2]);
    return argc == 1 ? 0 : 2;
}
