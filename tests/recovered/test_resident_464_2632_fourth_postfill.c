#include "poolrad/recovered/resident_464_2632_fourth_postfill.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    STACK_BASE = 0x1D16u << 4,
    TABLE_BASE = (0x14A0u << 4) + 0x25F6u,
    GLOBAL_WRITE = (0x14A0u << 4) + 0x4A28u + 0x003Eu,
    LOCAL_INDEX = STACK_BASE + 0x3E5Fu,
    LOCAL_COUNT = STACK_BASE + 0x3E5Du,
    LOCAL_OUTPUT = STACK_BASE + 0x3E64u,
    LOCAL_LOOP = STACK_BASE + 0x3E68u,
    LOCAL_LIMIT = STACK_BASE + 0x3E6Cu,
    LOCAL_DI = STACK_BASE + 0x3E62u,
    MASK = STACK_BASE + 0x3E8Eu
};

static const pr_r464_2632_cpu pre = {
    0x0C88, 0x2921, 0, 0x3E50, 0, 0, 0x2612, 0x3E68,
    0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7202
};
static const pr_r464_2632_cpu post = {
    0x0C88, 0x2968, 0, 0x3E50, 0, 0, 0x2612, 0x003E,
    0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7297
};

static void put_word(uint8_t *ram, size_t address, uint16_t value)
{
    ram[address] = (uint8_t)value;
    ram[address + 1u] = (uint8_t)(value >> 8);
}

static void seed(uint8_t *ram, uint8_t mask)
{
    unsigned i;
    memset(ram, 0xA5, PR_R464_2632_RAM_BYTES);
    for (i = 0; i < 4u; ++i) {
        ram[TABLE_BASE + i] = (uint8_t)(1u << i);
        ram[LOCAL_OUTPUT + i] = 0u;
    }
    ram[LOCAL_INDEX] = 0x7Fu;
    put_word(ram, LOCAL_COUNT, 0xCCCCu);
    put_word(ram, LOCAL_LOOP, 0xCCCCu);
    put_word(ram, LOCAL_LIMIT, 0x0029u);
    put_word(ram, LOCAL_DI, 0x003Eu);
    ram[MASK] = mask;
    ram[GLOBAL_WRITE] = 0x5Au;
}

typedef struct { unsigned count, errors; } observation;

static void observe(uint16_t ip, const uint8_t *ram,
                    const pr_r464_2632_cpu *cpu, void *context)
{
    observation *seen = context;
    (void)ram;
    if (cpu->cs != 0x0C88u || cpu->ip != ip ||
        ip < 0x2921u || ip > 0x2968u) ++seen->errors;
    ++seen->count;
}

static void test_mask(uint8_t mask, unsigned hits)
{
    uint8_t *ram = malloc(PR_R464_2632_RAM_BYTES);
    uint8_t *expected = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu = pre;
    observation seen = {0u, 0u};
    unsigned i;
    assert(ram != NULL && expected != NULL);
    seed(ram, mask);
    memcpy(expected, ram, PR_R464_2632_RAM_BYTES);
    expected[LOCAL_INDEX] = 3u;
    put_word(expected, LOCAL_COUNT, 0x0029u);
    put_word(expected, LOCAL_LOOP, 0u);
    for (i = 0; i < 4u; ++i)
        if ((mask & (1u << i)) != 0u) expected[LOCAL_OUTPUT + i] = 0xFFu;
    assert(pr_r464_2632_fourth_postfill(
               ram, PR_R464_2632_RAM_BYTES, &cpu, observe, &seen) ==
           PR_R464_2632_OK);
    assert(memcmp(ram, expected, PR_R464_2632_RAM_BYTES) == 0);
    assert(memcmp(&cpu, &post, sizeof(cpu)) == 0);
    assert(seen.count == 50u + 4u * hits && seen.errors == 0u);
    free(expected);
    free(ram);
}

static void test_invalid_and_unsupported(void)
{
    uint8_t *ram = malloc(PR_R464_2632_RAM_BYTES);
    uint8_t *copy = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu = pre, old;
    assert(ram != NULL && copy != NULL);
    seed(ram, 6u);
    memcpy(copy, ram, PR_R464_2632_RAM_BYTES);
    assert(pr_r464_2632_fourth_postfill(
               NULL, PR_R464_2632_RAM_BYTES, &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_postfill(
               ram, 655360u, &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_postfill(
               ram, PR_R464_2632_RAM_BYTES, NULL, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    cpu.ip = 0x2922u;
    old = cpu;
    assert(pr_r464_2632_fourth_postfill(
               ram, PR_R464_2632_RAM_BYTES, &cpu, NULL, NULL) ==
           PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(&cpu, &old, sizeof(cpu)) == 0);
    assert(memcmp(ram, copy, PR_R464_2632_RAM_BYTES) == 0);
    free(copy);
    free(ram);
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
    if (check->count >= 58u || fgets(line, sizeof(line), check->manifest) == NULL) {
        ++check->errors;
        ++check->count;
        return;
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
    status = pr_r464_2632_fourth_postfill(
        ram, PR_R464_2632_RAM_BYTES, &cpu, private_stage, &check);
    failed = status != PR_R464_2632_OK ||
             check.count != 58u || check.errors != 0u ||
             memcmp(&cpu, &post, sizeof(cpu)) != 0;
    printf("fourth +2921 post-FillChar checkpoints: %u errors: %u\n",
           check.count, check.errors);
    fclose(check.manifest);
    free(expected); free(ram);
    return failed;
}

int main(int argc, char **argv)
{
    test_mask(0u, 0u);
    test_mask(6u, 2u);
    test_mask(15u, 4u);
    test_invalid_and_unsupported();
    if (argc == 3) return test_replay(argv[1], argv[2]);
    return argc == 1 ? 0 : 2;
}
