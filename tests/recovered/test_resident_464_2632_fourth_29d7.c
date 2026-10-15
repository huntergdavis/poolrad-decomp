#include "poolrad/recovered/resident_464_2632_fourth_29d7.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    STACK_BASE = 0x1D16u << 4,
    TABLE_WORD = (0x14A0u << 4) + 0x4DECu
};

static const pr_r464_2632_cpu pre = {
    0x0C88, 0x29D7, 0x000B, 0x3E4E, 0, 0x14A0, 0x3E68, 0x14AB,
    0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246
};
static const uint16_t ips[6] = {
    0x29D7, 0x29DA, 0x29DB, 0x29DE, 0x29E0, 0x29E4
};

static void put_word(uint8_t *ram, size_t at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[at + 1u] = (uint8_t)(value >> 8);
}

static void seed(uint8_t *ram, uint16_t table)
{
    memset(ram, 0xA5, PR_R464_2632_RAM_BYTES);
    put_word(ram, STACK_BASE + 0x3E6Au, 0x148Cu);
    put_word(ram, STACK_BASE + 0x3E62u, 0x003Eu);
    put_word(ram, TABLE_WORD, table);
}

typedef struct { unsigned count, errors; uint16_t last; } observation;

static void observe(uint16_t ip, const uint8_t *ram,
                    const pr_r464_2632_cpu *cpu, void *context)
{
    observation *seen = context;
    (void)ram;
    if (seen->count >= 7u || cpu->cs != 0x0C88u ||
        cpu->ip != ip ||
        (seen->count < 6u && ip != ips[seen->count]))
        ++seen->errors;
    seen->last = ip;
    ++seen->count;
}

static void test_branch(uint16_t table, uint16_t target, uint16_t flags)
{
    uint8_t *ram = malloc(PR_R464_2632_RAM_BYTES);
    uint8_t *original = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu = pre;
    observation seen = {0u, 0u, 0u};
    assert(ram != NULL && original != NULL);
    seed(ram, table);
    memcpy(original, ram, PR_R464_2632_RAM_BYTES);
    assert(pr_r464_2632_fourth_29d7(
               ram, PR_R464_2632_RAM_BYTES, &cpu, observe, &seen) ==
           PR_R464_2632_OK);
    assert(memcmp(ram, original, PR_R464_2632_RAM_BYTES) == 0);
    assert(cpu.cs == 0x0C88u && cpu.ip == target &&
           cpu.ax == 0x148Bu && cpu.di == 0x007Cu &&
           cpu.flags == flags);
    assert(seen.count == 7u && seen.errors == 0u && seen.last == target);
    free(original); free(ram);
}

static void test_guard(void)
{
    uint8_t *ram = malloc(PR_R464_2632_RAM_BYTES);
    uint8_t *original = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu = pre, old;
    assert(ram != NULL && original != NULL);
    seed(ram, 0u);
    memcpy(original, ram, PR_R464_2632_RAM_BYTES);
    assert(pr_r464_2632_fourth_29d7(
               NULL, PR_R464_2632_RAM_BYTES, &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_29d7(
               ram, 655360u, &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_29d7(
               ram, PR_R464_2632_RAM_BYTES, NULL, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    cpu.ip = 0x29D8u;
    old = cpu;
    assert(pr_r464_2632_fourth_29d7(
               ram, PR_R464_2632_RAM_BYTES, &cpu, NULL, NULL) ==
           PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(&cpu, &old, sizeof(cpu)) == 0);
    assert(memcmp(ram, original, PR_R464_2632_RAM_BYTES) == 0);
    free(original); free(ram);
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
    if (check->count >= 7u || fgets(line, sizeof(line), check->manifest) == NULL) {
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
    status = pr_r464_2632_fourth_29d7(
        ram, PR_R464_2632_RAM_BYTES, &cpu, private_stage, &check);
    failed = status != PR_R464_2632_OK || check.count != 7u ||
             check.errors != 0u || cpu.ip != 0x29E6u ||
             cpu.flags != 0x7206u;
    printf("fourth +29D7 checkpoints: %u errors: %u\n",
           check.count, check.errors);
    fclose(check.manifest);
    free(expected); free(ram);
    return failed;
}

int main(int argc, char **argv)
{
    test_branch(0u, 0x29E6u, 0x7206u);
    test_branch(0x148Bu, 0x29F3u, 0x7246u);
    test_branch(0xFFFFu, 0x29F3u, 0x7213u);
    test_guard();
    if (argc == 3) return test_replay(argv[1], argv[2]);
    return argc == 1 ? 0 : 2;
}
