#include "poolrad/recovered/resident_464_2632_fourth_jg.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { PRIVATE_RAM_BYTES = 655360 };

static const pr_r464_2632_cpu pre_jg = {
    0x0C88, 0x275D, 168, 0, 104, 0, 0x2612, 8,
    0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246
};
static const pr_r464_2632_cpu saved_post = {
    0x0C88, 0x277D, 168, 0, 104, 0, 0x2612, 8,
    0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202
};

typedef struct {
    unsigned count, errors;
    uint16_t expected[3];
} stage_check;

static void stage(uint16_t ip, const uint8_t *ram,
                  const pr_r464_2632_cpu *cpu, void *context)
{
    stage_check *check = context;
    (void)ram;
    if (check->count >= 3u || ip != check->expected[check->count] ||
        cpu->ip != ip) ++check->errors;
    ++check->count;
}

static void test_synthetic(void)
{
    uint8_t *ram = malloc(PR_R464_2632_RAM_BYTES);
    uint8_t *before = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu, old;
    stage_check check = {0, 0, {0x275F, 0x2761, 0x277D}};
    assert(ram != NULL && before != NULL);
    memset(ram, 0xA5, PR_R464_2632_RAM_BYTES);
    memcpy(before, ram, PR_R464_2632_RAM_BYTES);
    cpu = pre_jg;
    assert(pr_r464_2632_fourth_jg(NULL, PR_R464_2632_RAM_BYTES,
                                   &cpu, NULL, NULL) == PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_jg(ram, PRIVATE_RAM_BYTES,
                                   &cpu, NULL, NULL) == PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_jg(ram, PR_R464_2632_RAM_BYTES,
                                   NULL, NULL, NULL) == PR_R464_2632_INVALID_HOST);
    cpu.ip = 0x275Fu;
    old = cpu;
    assert(pr_r464_2632_fourth_jg(ram, PR_R464_2632_RAM_BYTES,
                                   &cpu, NULL, NULL) == PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(&cpu, &old, sizeof(cpu)) == 0);
    cpu = pre_jg;
    assert(pr_r464_2632_fourth_jg(ram, PR_R464_2632_RAM_BYTES,
                                   &cpu, stage, &check) == PR_R464_2632_OK);
    assert(memcmp(&cpu, &saved_post, sizeof(cpu)) == 0);
    assert(check.count == 3u && check.errors == 0u);
    cpu = pre_jg;
    cpu.flags = 0x7202u; /* ZF=0, SF=OF=0: JG directly taken. */
    check.count = check.errors = 0u;
    check.expected[0] = 0x277Du;
    assert(pr_r464_2632_fourth_jg(ram, PR_R464_2632_RAM_BYTES,
                                   &cpu, stage, &check) == PR_R464_2632_OK);
    assert(cpu.ip == 0x277Du && cpu.flags == 0x7202u);
    assert(check.count == 1u && check.errors == 0u);
    cpu = pre_jg;
    cpu.ax = 50u; /* JG false; unsigned CMP below CX, JAE falls. */
    assert(pr_r464_2632_fourth_jg(ram, PR_R464_2632_RAM_BYTES,
                                   &cpu, NULL, NULL) == PR_R464_2632_OK);
    assert(cpu.ip == 0x2763u && (cpu.flags & 1u) != 0u);
    cpu = pre_jg;
    cpu.ax = cpu.cx; /* Equal comparison still takes JAE. */
    assert(pr_r464_2632_fourth_jg(ram, PR_R464_2632_RAM_BYTES,
                                   &cpu, NULL, NULL) == PR_R464_2632_OK);
    assert(cpu.ip == 0x277Du && (cpu.flags & 0x40u) != 0u);
    assert(memcmp(ram, before, PR_R464_2632_RAM_BYTES) == 0);
    free(before); free(ram);
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
    uint8_t *before;
    unsigned count, errors;
} private_check;

static void private_stage(uint16_t ip, const uint8_t *ram,
                          const pr_r464_2632_cpu *cpu, void *context)
{
    static const pr_r464_2632_cpu expected[] = {
        {0x0C88, 0x275F, 168, 0, 104, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246},
        {0x0C88, 0x2761, 168, 0, 104, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202},
        {0x0C88, 0x277D, 168, 0, 104, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202}
    };
    private_check *check = context;
    if (check->count >= 3u || ip != expected[check->count].ip ||
        memcmp(ram, check->before, PRIVATE_RAM_BYTES) != 0 ||
        memcmp(cpu, &expected[check->count], sizeof(*cpu)) != 0)
        ++check->errors;
    ++check->count;
}

static int test_replay(const char *pre_path)
{
    uint8_t *ram = calloc(PR_R464_2632_RAM_BYTES, 1);
    uint8_t *before = malloc(PRIVATE_RAM_BYTES);
    pr_r464_2632_cpu cpu = pre_jg;
    private_check check = {before, 0, 0};
    int status;
    if (ram == NULL || before == NULL ||
        !load_private(pre_path, ram)) { free(ram); free(before); return 1; }
    memcpy(before, ram, PRIVATE_RAM_BYTES);
    status = pr_r464_2632_fourth_jg(
        ram, PR_R464_2632_RAM_BYTES, &cpu, private_stage, &check);
    printf("fourth +275D branch RAM mismatches: %d stages: %u errors: %u\n",
           memcmp(ram, before, PRIVATE_RAM_BYTES) != 0,
           check.count, check.errors);
    status = status != PR_R464_2632_OK ||
             memcmp(&cpu, &saved_post, sizeof(cpu)) != 0 ||
             check.count != 3u || check.errors != 0u ||
             memcmp(ram, before, PRIVATE_RAM_BYTES) != 0;
    free(ram); free(before);
    return status;
}

int main(int argc, char **argv)
{
    test_synthetic();
    if (argc == 2) return test_replay(argv[1]);
    return argc == 1 ? 0 : 2;
}
