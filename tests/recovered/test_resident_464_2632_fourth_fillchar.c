#include "poolrad/recovered/resident_464_2632_fourth_fillchar.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    PRIVATE_RAM_BYTES = 655360,
    STACK_BASE = (0x1D16u << 4),
    ARG_VALUE = STACK_BASE + 0x3E54u,
    ARG_COUNT = STACK_BASE + 0x3E56u,
    ARG_OFFSET = STACK_BASE + 0x3E58u,
    ARG_SEGMENT = STACK_BASE + 0x3E5Au,
    RETURN_IP = STACK_BASE + 0x3E50u,
    RETURN_CS = STACK_BASE + 0x3E52u,
    FILL_START = STACK_BASE + 0x3E64u
};

static const pr_r464_2632_cpu pre_call = {
    0x0C88, 0x291C, 0, 0, 104, 0, 0x2612, 0x3E64,
    0x3E80, 0x3E54, 0x14A0, 0x3DE4, 0x1D16, 0x7202
};
static const pr_r464_2632_cpu saved_post = {
    0x0C88, 0x2921, 0, 0x3E50, 0, 0, 0x2612, 0x3E68,
    0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7202
};
static const uint16_t expected_cs[13] = {
    0x0C88, 0x131C, 0x131C, 0x131C, 0x131C, 0x131C, 0x131C,
    0x131C, 0x131C, 0x131C, 0x131C, 0x131C, 0x0C88
};
static const uint16_t expected_ip[13] = {
    0x291C, 0x16A8, 0x16AA, 0x16AE, 0x16B2, 0x16B6, 0x16B7,
    0x16B7, 0x16B7, 0x16B7, 0x16B7, 0x16B9, 0x2921
};

static void put_word(uint8_t *ram, size_t address, uint16_t value)
{
    ram[address] = (uint8_t)value;
    ram[address + 1u] = (uint8_t)(value >> 8);
}

static void seed(uint8_t *ram)
{
    unsigned i;
    memset(ram, 0xA5, PR_R464_2632_RAM_BYTES);
    put_word(ram, ARG_VALUE, 0u);
    put_word(ram, ARG_COUNT, 4u);
    put_word(ram, ARG_OFFSET, 0x3E64u);
    put_word(ram, ARG_SEGMENT, 0x1D16u);
    for (i = 0; i < 4u; ++i) ram[FILL_START + i] = (uint8_t)(0x81u + i);
}

typedef struct { unsigned count, errors; } stage_check;

static void stage(uint16_t ip, const uint8_t *ram,
                  const pr_r464_2632_cpu *cpu, void *context)
{
    stage_check *check = context;
    (void)ram;
    if (check->count >= 13u ||
        cpu->cs != expected_cs[check->count] ||
        ip != expected_ip[check->count] || cpu->ip != ip)
        ++check->errors;
    ++check->count;
}

static void test_synthetic(void)
{
    uint8_t *ram = malloc(PR_R464_2632_RAM_BYTES);
    uint8_t *expected = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu, old;
    stage_check check = {0u, 0u};
    unsigned i;
    assert(ram != NULL && expected != NULL);
    seed(ram);
    memcpy(expected, ram, PR_R464_2632_RAM_BYTES);
    cpu = pre_call;
    assert(pr_r464_2632_fourth_fillchar(NULL, PR_R464_2632_RAM_BYTES,
                                         &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_fillchar(ram, PRIVATE_RAM_BYTES,
                                         &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_fillchar(ram, PR_R464_2632_RAM_BYTES,
                                         NULL, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    cpu.ip = 0x291Du;
    old = cpu;
    assert(pr_r464_2632_fourth_fillchar(ram, PR_R464_2632_RAM_BYTES,
                                         &cpu, NULL, NULL) ==
           PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(&cpu, &old, sizeof(cpu)) == 0);
    assert(memcmp(ram, expected, PR_R464_2632_RAM_BYTES) == 0);
    cpu = pre_call;
    put_word(ram, ARG_COUNT, 5u);
    old = cpu;
    assert(pr_r464_2632_fourth_fillchar(ram, PR_R464_2632_RAM_BYTES,
                                         &cpu, NULL, NULL) ==
           PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(&cpu, &old, sizeof(cpu)) == 0);
    put_word(ram, ARG_COUNT, 4u);
    assert(memcmp(ram, expected, PR_R464_2632_RAM_BYTES) == 0);

    assert(pr_r464_2632_fourth_fillchar(ram, PR_R464_2632_RAM_BYTES,
                                         &cpu, stage, &check) ==
           PR_R464_2632_OK);
    put_word(expected, RETURN_IP, 0x2921u);
    put_word(expected, RETURN_CS, 0x0C88u);
    for (i = 0; i < 4u; ++i) expected[FILL_START + i] = 0u;
    assert(memcmp(ram, expected, PR_R464_2632_RAM_BYTES) == 0);
    assert(memcmp(&cpu, &saved_post, sizeof(cpu)) == 0);
    assert(check.count == 13u && check.errors == 0u);
    free(expected);
    free(ram);
}

static int load_private(const char *path, uint8_t *out)
{
    FILE *f = fopen(path, "rb");
    size_t n;
    if (f == NULL) return 0;
    n = fread(out, 1, PRIVATE_RAM_BYTES, f);
    if (fgetc(f) != EOF) { fclose(f); return 0; }
    fclose(f);
    return n == PRIVATE_RAM_BYTES;
}

typedef struct {
    const char *directory;
    uint8_t *expected_ram;
    unsigned count, errors;
} private_check;

static void private_stage(uint16_t ip, const uint8_t *ram,
                          const pr_r464_2632_cpu *cpu, void *context)
{
    private_check *check = context;
    pr_r464_2632_cpu expected = pre_call;
    char path[1024];
    int n;
    unsigned i = check->count;
    if (i >= 13u) { ++check->errors; ++check->count; return; }
    expected.cs = expected_cs[i];
    expected.ip = expected_ip[i];
    if (i >= 1u) expected.sp = 0x3E50u;
    if (i >= 2u) expected.bx = 0x3E50u;
    if (i >= 3u) expected.es = 0x1D16u;
    if (i >= 4u) expected.cx = 4u;
    if (i >= 7u && i <= 10u) {
        expected.cx = (uint16_t)(10u - i);
        expected.di = (uint16_t)(0x3E64u + i - 6u);
    }
    if (i >= 11u) { expected.cx = 0u; expected.di = 0x3E68u; }
    if (i == 12u) expected.sp = 0x3E5Cu;
    n = snprintf(path, sizeof(path), "%s/ram-%02u.bin",
                 check->directory, i);
    if (ip != expected.ip ||
        n < 0 || (size_t)n >= sizeof(path) ||
        !load_private(path, check->expected_ram) ||
        memcmp(ram, check->expected_ram, PRIVATE_RAM_BYTES) != 0 ||
        memcmp(cpu, &expected, sizeof(*cpu)) != 0)
        ++check->errors;
    ++check->count;
}

static int test_replay(const char *pre_path, const char *directory)
{
    uint8_t *ram = calloc(PR_R464_2632_RAM_BYTES, 1);
    uint8_t *expected = malloc(PRIVATE_RAM_BYTES);
    pr_r464_2632_cpu cpu = pre_call;
    private_check check = {directory, expected, 0u, 0u};
    int status;
    if (ram == NULL || expected == NULL || !load_private(pre_path, ram)) {
        free(ram); free(expected); return 1;
    }
    status = pr_r464_2632_fourth_fillchar(
        ram, PR_R464_2632_RAM_BYTES, &cpu, private_stage, &check);
    printf("fourth +291C FillChar checkpoints: %u errors: %u\n",
           check.count, check.errors);
    status = status != PR_R464_2632_OK ||
             memcmp(&cpu, &saved_post, sizeof(cpu)) != 0 ||
             check.count != 13u || check.errors != 0u;
    free(ram); free(expected);
    return status;
}

int main(int argc, char **argv)
{
    test_synthetic();
    if (argc == 3) return test_replay(argv[1], argv[2]);
    return argc == 1 ? 0 : 2;
}
