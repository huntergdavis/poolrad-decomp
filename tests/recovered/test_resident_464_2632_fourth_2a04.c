#include "poolrad/recovered/resident_464_2632_fourth_2a04.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { STACK_BASE = 0x1D16u << 4, DATA_BASE = 0x14A0u << 4 };

static const pr_r464_2632_cpu pre = {
    0x0C88, 0x2A04, 0, 0x3E4E, 0, 0x14A0, 0x3E68, 0x007C,
    0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7297
};
static const uint16_t ips[11] = {
    0x2A04, 0x2962, 0x2965, 0x2968, 0x296D, 0x2970,
    0x2973, 0x2976, 0x2978, 0x297C, 0x297F
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

static size_t mode_address(uint16_t index)
{
    return DATA_BASE + (uint16_t)(0x4A28u + index);
}

static size_t table_address(uint16_t index)
{
    return DATA_BASE + (uint16_t)(0x4C20u + (uint16_t)(index << 1));
}

static void seed(uint8_t *ram, uint16_t counter, uint16_t index,
                 uint16_t source, uint16_t accumulated, uint16_t table)
{
    memset(ram, 0xA5, PR_R464_2632_RAM_BYTES);
    put_word(ram, STACK_BASE + 0x3E68u, counter);
    put_word(ram, STACK_BASE + 0x3E62u, index);
    put_word(ram, STACK_BASE + 0x3E72u, source);
    put_word(ram, STACK_BASE + 0x3E6Au, accumulated);
    put_word(ram, table_address(index), table);
}

typedef struct {
    unsigned count, errors;
    uint16_t last, inc_flags, add_flags, shl_flags, cmp_flags;
} observation;

static void observe(uint16_t ip, const uint8_t *ram,
                    const pr_r464_2632_cpu *cpu, void *context)
{
    observation *seen = context;
    (void)ram;
    if (seen->count >= 12u || cpu->cs != 0x0C88u || cpu->ip != ip ||
        (seen->count < 11u && ip != ips[seen->count]))
        ++seen->errors;
    if ((seen->count == 2u && cpu->flags != seen->inc_flags) ||
        (seen->count == 6u && cpu->flags != seen->add_flags) ||
        (seen->count == 8u && cpu->flags != seen->shl_flags) ||
        (seen->count == 10u && cpu->flags != seen->cmp_flags))
        ++seen->errors;
    seen->last = ip;
    ++seen->count;
}

static void test_branch(uint16_t counter, uint16_t index,
                        uint16_t source, uint16_t accumulated,
                        uint16_t table, uint16_t next,
                        uint16_t inc_flags, uint16_t add_flags,
                        uint16_t shl_flags, uint16_t cmp_flags)
{
    uint8_t *ram = malloc(PR_R464_2632_RAM_BYTES);
    uint8_t *expected_ram = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu = pre, expected_cpu = pre;
    observation seen = {0u, 0u, 0u, inc_flags, add_flags,
                        shl_flags, cmp_flags};
    assert(ram != NULL && expected_ram != NULL);
    seed(ram, counter, index, source, accumulated, table);
    memcpy(expected_ram, ram, PR_R464_2632_RAM_BYTES);
    put_word(expected_ram, STACK_BASE + 0x3E68u,
             (uint16_t)(counter + 1u));
    expected_ram[mode_address(index)] = 1u;
    put_word(expected_ram, STACK_BASE + 0x3E6Au,
             (uint16_t)(accumulated + source));
    expected_cpu.ip = next;
    expected_cpu.ax = table;
    expected_cpu.di = (uint16_t)(index << 1);
    expected_cpu.flags = cmp_flags;
    assert(pr_r464_2632_fourth_2a04(
               ram, PR_R464_2632_RAM_BYTES, &cpu, observe, &seen) ==
           PR_R464_2632_OK);
    assert(memcmp(ram, expected_ram, PR_R464_2632_RAM_BYTES) == 0);
    assert(memcmp(&cpu, &expected_cpu, sizeof(cpu)) == 0);
    assert(word_at(ram, STACK_BASE + 0x3E68u) ==
           (uint16_t)(counter + 1u));
    assert(word_at(ram, STACK_BASE + 0x3E6Au) ==
           (uint16_t)(accumulated + source));
    assert(ram[mode_address(index)] == 1u);
    assert(seen.count == 12u && seen.errors == 0u && seen.last == next);
    free(expected_ram); free(ram);
}

static void test_guard(void)
{
    uint8_t *ram = malloc(PR_R464_2632_RAM_BYTES);
    uint8_t *copy = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu = pre, old;
    assert(ram != NULL && copy != NULL);
    seed(ram, 0u, 0x3Fu, 8u, 0x14ACu, 0xFFFFu);
    memcpy(copy, ram, PR_R464_2632_RAM_BYTES);
    assert(pr_r464_2632_fourth_2a04(
               NULL, PR_R464_2632_RAM_BYTES, &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_2a04(
               ram, 655360u, &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_2a04(
               ram, PR_R464_2632_RAM_BYTES, NULL, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    cpu.ip = 0x2A05u;
    old = cpu;
    assert(pr_r464_2632_fourth_2a04(
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
    if (check->count >= 12u || fgets(line, sizeof(line), check->manifest) == NULL) {
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
    status = pr_r464_2632_fourth_2a04(
        ram, PR_R464_2632_RAM_BYTES, &cpu, private_stage, &check);
    failed = status != PR_R464_2632_OK || check.count != 12u ||
             check.errors != 0u || cpu.ip != 0x2981u ||
             cpu.ax != 0xFFFFu || cpu.flags != 0x7286u;
    printf("fourth +2A04 checkpoints: %u errors: %u\n",
           check.count, check.errors);
    fclose(check.manifest);
    free(expected); free(ram);
    return failed;
}

int main(int argc, char **argv)
{
    test_branch(0u, 0x3Fu, 8u, 0x14ACu, 0xFFFFu,
                0x2981u, 0x7203u, 0x7216u, 0x7206u, 0x7286u);
    test_branch(0u, 0x3Fu, 8u, 0x14ACu, 0x14B4u,
                0x2999u, 0x7203u, 0x7216u, 0x7206u, 0x7246u);
    test_branch(0u, 0x3Fu, 8u, 0x14ACu, 0u,
                0x2999u, 0x7203u, 0x7216u, 0x7206u, 0x7293u);
    test_branch(0xFFFFu, 0x8000u, 1u, 0xFFFFu, 0xFFFFu,
                0x2981u, 0x7257u, 0x7257u, 0x7A47u, 0x7286u);
    test_guard();
    if (argc == 3) return test_replay(argv[1], argv[2]);
    return argc == 1 ? 0 : 2;
}
