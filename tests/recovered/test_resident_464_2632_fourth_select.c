#include "poolrad/recovered/resident_464_2632_fourth_select.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    PRIVATE_RAM_BYTES = 655360,
    SOURCE = (0x1D16u << 4) + 0x3E90u,
    TARGET = (0x1D16u << 4) + 0x3E6Cu,
    SELECTOR = (0x14A0u << 4) + 0x4A1Eu
};

static const pr_r464_2632_cpu pre_select = {
    0x0C88, 0x277D, 168, 0, 104, 0, 0x2612, 8,
    0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202
};
static const pr_r464_2632_cpu saved_post = {
    0x0C88, 0x28D9, 1, 0, 104, 0, 0x2612, 8,
    0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7297
};

typedef struct {
    const uint16_t *expected;
    unsigned length, count, errors;
} stage_check;

static void stage(uint16_t ip, const uint8_t *ram,
                  const pr_r464_2632_cpu *cpu, void *context)
{
    stage_check *check = context;
    (void)ram;
    if (check->count >= check->length ||
        ip != check->expected[check->count] || cpu->ip != ip)
        ++check->errors;
    ++check->count;
}

static void seed(uint8_t *ram, uint8_t choice)
{
    memset(ram, 0xA5, PR_R464_2632_RAM_BYTES);
    ram[SOURCE] = 42u;
    ram[SOURCE + 1u] = 0u;
    ram[TARGET] = 0xEFu;
    ram[TARGET + 1u] = 0xBEu;
    ram[SELECTOR] = choice;
}

static void test_synthetic(void)
{
    static const uint16_t miss_path[] = {
        0x2780u, 0x2781u, 0x2784u, 0x2787u, 0x2789u,
        0x278Bu, 0x278Du, 0x278Fu, 0x28D9u
    };
    static const uint16_t zero_path[] = {
        0x2780u, 0x2781u, 0x2784u, 0x2787u, 0x2789u, 0x2792u
    };
    static const uint16_t two_path[] = {
        0x2780u, 0x2781u, 0x2784u, 0x2787u, 0x2789u,
        0x278Bu, 0x278Du, 0x2792u
    };
    uint8_t *ram = malloc(PR_R464_2632_RAM_BYTES);
    uint8_t *expected = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu, old;
    stage_check check = {miss_path, 9u, 0u, 0u};
    assert(ram != NULL && expected != NULL);
    seed(ram, 1u);
    memcpy(expected, ram, PR_R464_2632_RAM_BYTES);
    cpu = pre_select;
    assert(pr_r464_2632_fourth_select(NULL, PR_R464_2632_RAM_BYTES,
                                       &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_select(ram, PRIVATE_RAM_BYTES,
                                       &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_select(ram, PR_R464_2632_RAM_BYTES,
                                       NULL, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    cpu.ip = 0x277Eu;
    old = cpu;
    assert(pr_r464_2632_fourth_select(ram, PR_R464_2632_RAM_BYTES,
                                       &cpu, NULL, NULL) ==
           PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(&cpu, &old, sizeof(cpu)) == 0);
    assert(memcmp(ram, expected, PR_R464_2632_RAM_BYTES) == 0);

    cpu = pre_select;
    assert(pr_r464_2632_fourth_select(ram, PR_R464_2632_RAM_BYTES,
                                       &cpu, stage, &check) ==
           PR_R464_2632_OK);
    expected[TARGET] = 41u;
    expected[TARGET + 1u] = 0u;
    assert(memcmp(ram, expected, PR_R464_2632_RAM_BYTES) == 0);
    assert(memcmp(&cpu, &saved_post, sizeof(cpu)) == 0);
    assert(check.count == 9u && check.errors == 0u);

    seed(ram, 0u);
    cpu = pre_select;
    check = (stage_check){zero_path, 6u, 0u, 0u};
    assert(pr_r464_2632_fourth_select(ram, PR_R464_2632_RAM_BYTES,
                                       &cpu, stage, &check) ==
           PR_R464_2632_OK);
    assert(cpu.ip == 0x2792u && cpu.ax == 0u && cpu.flags == 0x7246u);
    assert(ram[TARGET] == 41u && ram[TARGET + 1u] == 0u);
    assert(check.count == 6u && check.errors == 0u);

    seed(ram, 2u);
    cpu = pre_select;
    check = (stage_check){two_path, 8u, 0u, 0u};
    assert(pr_r464_2632_fourth_select(ram, PR_R464_2632_RAM_BYTES,
                                       &cpu, stage, &check) ==
           PR_R464_2632_OK);
    assert(cpu.ip == 0x2792u && cpu.ax == 2u && cpu.flags == 0x7246u);
    assert(check.count == 8u && check.errors == 0u);

    seed(ram, 3u);
    cpu = pre_select;
    assert(pr_r464_2632_fourth_select(ram, PR_R464_2632_RAM_BYTES,
                                       &cpu, NULL, NULL) ==
           PR_R464_2632_OK);
    assert(cpu.ip == 0x28D9u && cpu.ax == 3u && cpu.flags == 0x7202u);

    seed(ram, 1u);
    ram[SOURCE] = ram[SOURCE + 1u] = 0u;
    cpu = pre_select;
    assert(pr_r464_2632_fourth_select(ram, PR_R464_2632_RAM_BYTES,
                                       &cpu, NULL, NULL) ==
           PR_R464_2632_OK);
    assert(cpu.ip == 0x28D9u && cpu.ax == 0xFF01u);
    assert(ram[TARGET] == 0xFFu && ram[TARGET + 1u] == 0xFFu);
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
    const uint8_t *before;
    unsigned count, errors;
} private_check;

static void private_stage(uint16_t ip, const uint8_t *ram,
                          const pr_r464_2632_cpu *cpu, void *context)
{
    static const pr_r464_2632_cpu expected[] = {
        {0x0C88, 0x2780, 42, 0, 104, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202},
        {0x0C88, 0x2781, 41, 0, 104, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202},
        {0x0C88, 0x2784, 41, 0, 104, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202},
        {0x0C88, 0x2787, 1, 0, 104, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202},
        {0x0C88, 0x2789, 1, 0, 104, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202},
        {0x0C88, 0x278B, 1, 0, 104, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202},
        {0x0C88, 0x278D, 1, 0, 104, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7297},
        {0x0C88, 0x278F, 1, 0, 104, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7297},
        {0x0C88, 0x28D9, 1, 0, 104, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7297}
    };
    private_check *check = context;
    if (check->count >= sizeof(expected) / sizeof(expected[0]) ||
        ip != expected[check->count].ip ||
        memcmp(ram, check->before, PRIVATE_RAM_BYTES) != 0 ||
        memcmp(cpu, &expected[check->count], sizeof(*cpu)) != 0)
        ++check->errors;
    ++check->count;
}

static int test_replay(const char *pre_path)
{
    uint8_t *ram = calloc(PR_R464_2632_RAM_BYTES, 1);
    uint8_t *before = malloc(PRIVATE_RAM_BYTES);
    pr_r464_2632_cpu cpu = pre_select;
    private_check check = {before, 0u, 0u};
    int status;
    if (ram == NULL || before == NULL || !load_private(pre_path, ram)) {
        free(ram); free(before); return 1;
    }
    memcpy(before, ram, PRIVATE_RAM_BYTES);
    status = pr_r464_2632_fourth_select(
        ram, PR_R464_2632_RAM_BYTES, &cpu, private_stage, &check);
    printf("fourth +277D selector RAM mismatches: %d stages: %u errors: %u\n",
           memcmp(ram, before, PRIVATE_RAM_BYTES) != 0,
           check.count, check.errors);
    status = status != PR_R464_2632_OK ||
             memcmp(&cpu, &saved_post, sizeof(cpu)) != 0 ||
             check.count != 9u || check.errors != 0u ||
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
