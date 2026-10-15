#include "poolrad/recovered/resident_464_2632_fourth_29ab.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    STACK_BASE = 0x1D16u << 4,
    GLOBAL_BYTE = (0x14A0u << 4) + 0x70C7u
};

static const pr_r464_2632_cpu pre = {
    0x0C88, 0x29AB, 0x0001, 0x3E4E, 0, 0x14A0, 0x3E68, 0x1483,
    0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7297
};
static const pr_r464_2632_cpu saved_post = {
    0x0C88, 0x29C6, 0x0004, 0x3E4E, 0, 0x14A0, 0x3E68, 0x1483,
    0x3E80, 0x3E52, 0x14A0, 0x3DE4, 0x1D16, 0x7212
};
static const uint16_t ips[13] = {
    0x29AB, 0x29AF, 0x29B2, 0x29B3, 0x29B4, 0x29B7,
    0x29BA, 0x29BC, 0x29C0, 0x29C1, 0x29C2, 0x29C5, 0x29C6
};

static void put_word(uint8_t *ram, size_t at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[at + 1u] = (uint8_t)(value >> 8);
}

static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[at + 1u] << 8));
}

static void seed(uint8_t *ram)
{
    memset(ram, 0xA5, PR_R464_2632_RAM_BYTES);
    put_word(ram, STACK_BASE + 0x3E6Au, 0x1464u);
    put_word(ram, STACK_BASE + 0x3E86u, 0x0008u);
    put_word(ram, STACK_BASE + 0x3E88u, 0x3DE4u);
    ram[GLOBAL_BYTE] = 1u;
}

typedef struct { unsigned count, errors; uint16_t inc_flags; } observation;

static void observe(uint16_t ip, const uint8_t *ram,
                    const pr_r464_2632_cpu *cpu, void *context)
{
    observation *seen = context;
    (void)ram;
    if (seen->count >= 13u || ip != ips[seen->count] ||
        cpu->cs != 0x0C88u || cpu->ip != ip)
        ++seen->errors;
    if (seen->count == 1u) seen->inc_flags = cpu->flags;
    ++seen->count;
}

static void test_synthetic(void)
{
    uint8_t *ram = malloc(PR_R464_2632_RAM_BYTES);
    uint8_t *expected = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu = pre;
    observation seen = {0u, 0u, 0u};
    assert(ram != NULL && expected != NULL);
    seed(ram);
    memcpy(expected, ram, PR_R464_2632_RAM_BYTES);
    expected[GLOBAL_BYTE] = 2u;
    put_word(expected, STACK_BASE + 0x3E5Au, 0x1D16u);
    put_word(expected, STACK_BASE + 0x3E58u, 0x3E64u);
    put_word(expected, STACK_BASE + 0x3E56u, 0x3DE4u);
    put_word(expected, STACK_BASE + 0x3E54u, 0x1483u);
    put_word(expected, STACK_BASE + 0x3E52u, 4u);
    assert(pr_r464_2632_fourth_29ab(
               ram, PR_R464_2632_RAM_BYTES, &cpu, observe, &seen) ==
           PR_R464_2632_OK);
    assert(memcmp(ram, expected, PR_R464_2632_RAM_BYTES) == 0);
    assert(memcmp(&cpu, &saved_post, sizeof(cpu)) == 0);
    assert(word_at(ram, STACK_BASE + 0x3E52u) == 4u);
    assert(seen.count == 13u && seen.errors == 0u &&
           seen.inc_flags == 0x7203u);
    free(expected); free(ram);
}

static void test_increment_wrap(void)
{
    uint8_t *ram = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu = pre;
    observation seen = {0u, 0u, 0u};
    assert(ram != NULL);
    seed(ram);
    ram[GLOBAL_BYTE] = 0xFFu;
    assert(pr_r464_2632_fourth_29ab(
               ram, PR_R464_2632_RAM_BYTES, &cpu, observe, &seen) ==
           PR_R464_2632_OK);
    assert(ram[GLOBAL_BYTE] == 0u);
    assert(seen.count == 13u && seen.errors == 0u &&
           seen.inc_flags == 0x7257u);
    assert(memcmp(&cpu, &saved_post, sizeof(cpu)) == 0);
    free(ram);
}

static void test_guard(void)
{
    uint8_t *ram = malloc(PR_R464_2632_RAM_BYTES);
    uint8_t *copy = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu = pre, old;
    assert(ram != NULL && copy != NULL);
    seed(ram);
    memcpy(copy, ram, PR_R464_2632_RAM_BYTES);
    assert(pr_r464_2632_fourth_29ab(
               NULL, PR_R464_2632_RAM_BYTES, &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_29ab(
               ram, 655360u, &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_29ab(
               ram, PR_R464_2632_RAM_BYTES, NULL, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    cpu.ip = 0x29A9u;
    old = cpu;
    assert(pr_r464_2632_fourth_29ab(
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
    if (check->count >= 13u || fgets(line, sizeof(line), check->manifest) == NULL) {
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
    status = pr_r464_2632_fourth_29ab(
        ram, PR_R464_2632_RAM_BYTES, &cpu, private_stage, &check);
    failed = status != PR_R464_2632_OK || check.count != 13u ||
             check.errors != 0u || memcmp(&cpu, &saved_post, sizeof(cpu)) != 0;
    printf("fourth +29AB checkpoints: %u errors: %u\n",
           check.count, check.errors);
    fclose(check.manifest);
    free(expected); free(ram);
    return failed;
}

int main(int argc, char **argv)
{
    test_synthetic();
    test_increment_wrap();
    test_guard();
    if (argc == 3) return test_replay(argv[1], argv[2]);
    return argc == 1 ? 0 : 2;
}
