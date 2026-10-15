#include "poolrad/recovered/resident_464_2632_fourth_precall.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { PRIVATE_RAM_BYTES = 655360 };

static const pr_r464_2632_cpu pre_call = {
    0x0C88, 0x28D9, 1, 0, 104, 0, 0x2612, 8,
    0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7297
};
static const pr_r464_2632_cpu saved_post = {
    0x0C88, 0x291C, 0, 0, 104, 0, 0x2612, 0x3E64,
    0x3E80, 0x3E54, 0x14A0, 0x3DE4, 0x1D16, 0x7202
};
static const uint16_t saved_ips[] = {
    0x28DB, 0x28E0, 0x28E3, 0x28E6, 0x28E9, 0x28EC, 0x28EF,
    0x28F2, 0x28F5, 0x28F7, 0x28F9, 0x28FC, 0x28FF, 0x2901,
    0x2903, 0x2906, 0x2909, 0x290B, 0x290D, 0x2910, 0x2913,
    0x2914, 0x2915, 0x2918, 0x2919, 0x291B, 0x291C
};

static size_t stack_at(int delta)
{
    return ((size_t)0x1D16u << 4) + (size_t)(0x3E80 + delta);
}

static void put_word(uint8_t *ram, size_t address, uint16_t value)
{
    ram[address] = (uint8_t)value;
    ram[address + 1u] = (uint8_t)(value >> 8);
}

static uint16_t word_at(const uint8_t *ram, size_t address)
{
    return (uint16_t)(ram[address] | ((uint16_t)ram[address + 1u] << 8));
}

typedef struct {
    const uint16_t *ips;
    unsigned length, count, errors;
} stage_check;

static void stage(uint16_t ip, const uint8_t *ram,
                  const pr_r464_2632_cpu *cpu, void *context)
{
    stage_check *check = context;
    (void)ram;
    if (check->count >= check->length ||
        ip != check->ips[check->count] || cpu->ip != ip)
        ++check->errors;
    ++check->count;
}

static void seed(uint8_t *ram)
{
    memset(ram, 0xA5, PR_R464_2632_RAM_BYTES);
    put_word(ram, stack_at(-0x0E), 2u);
    put_word(ram, stack_at(+0x12), 11u);
    put_word(ram, stack_at(-0x0A), 0u);
    put_word(ram, stack_at(-0x0C), 0u);
    put_word(ram, stack_at(-0x10), 8u);
    put_word(ram, stack_at(-0x16), 0x0516u);
}

static void test_synthetic(void)
{
    static const uint16_t other_ips[] = {0x28DBu, 0x28DDu};
    uint8_t *ram = malloc(PR_R464_2632_RAM_BYTES);
    uint8_t *expected = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu, old;
    stage_check check = {saved_ips, 27u, 0u, 0u};
    assert(ram != NULL && expected != NULL);
    seed(ram);
    memcpy(expected, ram, PR_R464_2632_RAM_BYTES);
    cpu = pre_call;
    assert(pr_r464_2632_fourth_precall(NULL, PR_R464_2632_RAM_BYTES,
                                        &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_precall(ram, PRIVATE_RAM_BYTES,
                                        &cpu, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    assert(pr_r464_2632_fourth_precall(ram, PR_R464_2632_RAM_BYTES,
                                        NULL, NULL, NULL) ==
           PR_R464_2632_INVALID_HOST);
    cpu.ip = 0x28DAu;
    old = cpu;
    assert(pr_r464_2632_fourth_precall(ram, PR_R464_2632_RAM_BYTES,
                                        &cpu, NULL, NULL) ==
           PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(&cpu, &old, sizeof(cpu)) == 0);
    assert(memcmp(ram, expected, PR_R464_2632_RAM_BYTES) == 0);
    cpu = pre_call;
    put_word(ram, stack_at(-0x0E), 3u);
    old = cpu;
    assert(pr_r464_2632_fourth_precall(ram, PR_R464_2632_RAM_BYTES,
                                        &cpu, NULL, NULL) ==
           PR_R464_2632_UNSUPPORTED_PATH);
    assert(memcmp(&cpu, &old, sizeof(cpu)) == 0);
    assert(word_at(ram, stack_at(-0x0E)) == 3u);
    put_word(ram, stack_at(-0x0E), 2u);

    assert(pr_r464_2632_fourth_precall(ram, PR_R464_2632_RAM_BYTES,
                                        &cpu, stage, &check) ==
           PR_R464_2632_OK);
    put_word(expected, stack_at(-0x20), 2u);
    put_word(expected, stack_at(-0x12), 11u);
    put_word(expected, stack_at(-0x0E), 8u);
    put_word(expected, stack_at(-0x10), 32u);
    put_word(expected, stack_at(-0x16), 0x1458u);
    put_word(expected, ((size_t)0x1D16u << 4) + 0x3E5Au, 0x1D16u);
    put_word(expected, ((size_t)0x1D16u << 4) + 0x3E58u, 0x3E64u);
    put_word(expected, ((size_t)0x1D16u << 4) + 0x3E56u, 4u);
    put_word(expected, ((size_t)0x1D16u << 4) + 0x3E54u, 0u);
    assert(memcmp(ram, expected, PR_R464_2632_RAM_BYTES) == 0);
    assert(memcmp(&cpu, &saved_post, sizeof(cpu)) == 0);
    assert(check.count == 27u && check.errors == 0u);

    seed(ram);
    memcpy(expected, ram, PR_R464_2632_RAM_BYTES);
    cpu = pre_call;
    cpu.ax = 3u;
    check = (stage_check){other_ips, 2u, 0u, 0u};
    assert(pr_r464_2632_fourth_precall(ram, PR_R464_2632_RAM_BYTES,
                                        &cpu, stage, &check) ==
           PR_R464_2632_OK);
    assert(cpu.ip == 0x28DDu && cpu.ax == 3u && cpu.flags == 0x7202u);
    assert(check.count == 2u && check.errors == 0u);
    assert(memcmp(ram, expected, PR_R464_2632_RAM_BYTES) == 0);
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
    stage_check stages;
    const char *directory;
    uint8_t *expected_ram;
    unsigned next, errors;
} private_check;

static void private_stage(uint16_t ip, const uint8_t *ram,
                          const pr_r464_2632_cpu *cpu, void *context)
{
    static const pr_r464_2632_cpu expected[] = {
        {0x0C88, 0x28DB, 1, 0, 104, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246},
        {0x0C88, 0x28E0, 1, 0, 104, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246},
        {0x0C88, 0x28FC, 8, 0, 104, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202},
        {0x0C88, 0x2906, 32, 0, 104, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202},
        {0x0C88, 0x2910, 0x1458, 0, 104, 0, 0x2612, 8,
         0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202},
        {0x0C88, 0x2914, 0x1458, 0, 104, 0, 0x2612, 0x3E64,
         0x3E80, 0x3E5A, 0x14A0, 0x3DE4, 0x1D16, 0x7202},
        {0x0C88, 0x291C, 0, 0, 104, 0, 0x2612, 0x3E64,
         0x3E80, 0x3E54, 0x14A0, 0x3DE4, 0x1D16, 0x7202}
    };
    private_check *check = context;
    char path[1024];
    int n;
    stage(ip, ram, cpu, &check->stages);
    if (check->next >= sizeof(expected) / sizeof(expected[0]) ||
        ip != expected[check->next].ip)
        return;
    n = snprintf(path, sizeof(path), "%s/ram-%04x.bin",
                 check->directory, ip);
    if (n < 0 || (size_t)n >= sizeof(path) ||
        !load_private(path, check->expected_ram) ||
        memcmp(ram, check->expected_ram, PRIVATE_RAM_BYTES) != 0 ||
        memcmp(cpu, &expected[check->next], sizeof(*cpu)) != 0)
        ++check->errors;
    ++check->next;
}

static int test_replay(const char *pre_path, const char *snapshot_dir)
{
    uint8_t *ram = calloc(PR_R464_2632_RAM_BYTES, 1);
    uint8_t *expected = malloc(PRIVATE_RAM_BYTES);
    pr_r464_2632_cpu cpu = pre_call;
    private_check check = {{saved_ips, 27u, 0u, 0u},
                           snapshot_dir, expected, 0u, 0u};
    int status;
    if (ram == NULL || expected == NULL || !load_private(pre_path, ram)) {
        free(ram); free(expected); return 1;
    }
    status = pr_r464_2632_fourth_precall(
        ram, PR_R464_2632_RAM_BYTES, &cpu, private_stage, &check);
    printf("fourth +28D9 precall checkpoints: %u stages: %u errors: %u\n",
           check.next, check.stages.count,
           check.errors + check.stages.errors);
    status = status != PR_R464_2632_OK ||
             memcmp(&cpu, &saved_post, sizeof(cpu)) != 0 ||
             check.stages.count != 27u || check.stages.errors != 0u ||
             check.next != 7u || check.errors != 0u;
    free(ram); free(expected);
    return status;
}

int main(int argc, char **argv)
{
    test_synthetic();
    if (argc == 3) return test_replay(argv[1], argv[2]);
    return argc == 1 ? 0 : 2;
}
