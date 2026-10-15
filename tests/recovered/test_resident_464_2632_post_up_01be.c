#include "poolrad/recovered/resident_464_2632_run014.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d\n", __LINE__); \
        return 1; \
    } \
} while (0)

enum {
    CAPTURE_BYTES = 0xA0000u,
    DS = 0x14A00u,
    SS = 0x1D160u,
    OBJECT = 0x3DE48u,
    OBJECT_BYTES = 14135u,
    ROW = 59u,
    ROW_SLOT = 118u,
    BP = 0x3EAAu
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

static pr_r464_2632_cpu entry_cpu(void)
{
    return (pr_r464_2632_cpu){
        0x0C88u, 0x2632u, 0x000Fu, 0x3E78u, 0u, 0x14A0u, 0x3E92u,
        0x260Au, 0x3EC2u, 0x3EACu, 0x14A0u, 0x3DE4u, 0x1D16u, 0x7246u
    };
}

static pr_r464_2632_cpu return_cpu(void)
{
    return (pr_r464_2632_cpu){
        0x2D14u, 0x01C3u, 0u, 0x3E78u, 0u, 0x14A0u, 0x3E92u,
        0x0076u, 0x3EC2u, 0x3EC2u, 0x14A0u, 0x3DE4u, 0x1D16u, 0x7246u
    };
}

static void setup(uint8_t *ram)
{
    unsigned row;
    memset(ram, 0, PR_R464_2632_RAM_BYTES);
    memset(ram + OBJECT + 23u, 0xA5, OBJECT_BYTES - 23u);
    put_word(ram, OBJECT, 168u);
    put_word(ram, OBJECT + 2u, 21u);
    put_word(ram, SS + BP, 0x3EC2u);
    put_word(ram, SS + BP + 2u, 0x01C3u);
    put_word(ram, SS + BP + 4u, 0x2D14u);
    put_word(ram, SS + BP + 6u, 8u);
    put_word(ram, SS + BP + 8u, 0x3DE4u);
    put_word(ram, SS + BP + 0x0Au, 0x260Au);
    put_word(ram, SS + BP + 0x0Cu, 0x14A0u);
    put_word(ram, SS + BP + 0x0Eu, 0x000Fu);
    put_word(ram, SS + BP + 0x10u, 1u);
    put_word(ram, SS + BP + 0x12u, 11u);
    put_word(ram, SS + BP + 0x14u, ROW);
    put_word(ram, SS + BP + 0x16u, 2u);
    ram[DS + 0x4A1Eu] = 1u;
    ram[DS + 0x25F6u] = 1u;
    ram[DS + 0x25F7u] = 2u;
    ram[DS + 0x25F8u] = 4u;
    ram[DS + 0x25F9u] = 8u;
    for (row = 0u; row < 168u; ++row)
        put_word(ram, DS + 0x4C20u + 2u * row, 0xFFFFu);
    for (row = 16u; row <= 58u; ++row) {
        ram[DS + 0x4A28u + row] = 1u;
        put_word(ram, DS + 0x4AD0u + 2u * row, 2u);
        put_word(ram, DS + 0x4C20u + 2u * row,
                 (uint16_t)((row * 21u + 2u) * 4u));
        put_word(ram, DS + 0x4D70u + 2u * row,
                 (uint16_t)((row * 21u + 2u) * 4u + 43u));
    }
}

static int synthetic(void)
{
    uint8_t *ram = malloc(PR_R464_2632_RAM_BYTES);
    uint8_t *before = malloc(PR_R464_2632_RAM_BYTES);
    pr_r464_2632_cpu cpu = entry_cpu();
    pr_r464_2632_cpu final = return_cpu();
    size_t row_start = OBJECT + 23u + ROW * 21u * 4u;
    unsigned col, row;
    CHECK(ram != NULL && before != NULL);
    setup(ram);
    memcpy(before, ram, PR_R464_2632_RAM_BYTES);
    CHECK(pr_r464_2632_run014(ram, PR_R464_2632_RAM_BYTES,
                               &cpu, NULL, NULL) == PR_R464_2632_OK);
    CHECK(memcmp(&cpu, &final, sizeof(cpu)) == 0);
    CHECK(memcmp(ram + OBJECT, before + OBJECT, 23u) == 0);
    CHECK(memcmp(ram + OBJECT, before + OBJECT, row_start - OBJECT) == 0);
    for (col = 0u; col < 21u; ++col) {
        size_t at = row_start + col * 4u;
        if (col >= 2u && col <= 12u) {
            CHECK(ram[at] == 0xFFu && ram[at + 1u] == 0xFFu);
            CHECK(ram[at + 2u] == 0xFFu && ram[at + 3u] == 0xFFu);
        } else {
            CHECK(memcmp(ram + at, before + at, 4u) == 0);
        }
    }
    CHECK(memcmp(ram + row_start + 21u * 4u,
                 before + row_start + 21u * 4u,
                 OBJECT_BYTES - (row_start - OBJECT) - 21u * 4u) == 0);
    for (row = 0u; row < 168u; ++row)
        CHECK(ram[DS + 0x4A28u + row] ==
              (uint8_t)(row >= 16u && row <= 59u));
    CHECK(word_at(ram, DS + 0x4AD0u + ROW_SLOT) == 2u);
    CHECK(word_at(ram, DS + 0x4C20u + ROW_SLOT) == 4964u);
    CHECK(word_at(ram, DS + 0x4D70u + ROW_SLOT) == 5007u);
    CHECK(ram[DS + 0x70C7u] == 11u);
    setup(ram);
    put_word(ram, SS + BP + 0x10u, 43u);
    memcpy(before, ram, PR_R464_2632_RAM_BYTES);
    cpu = entry_cpu();
    CHECK(pr_r464_2632_run014(ram, PR_R464_2632_RAM_BYTES,
                               &cpu, NULL, NULL) ==
          PR_R464_2632_UNSUPPORTED_PATH);
    CHECK(memcmp(ram, before, PR_R464_2632_RAM_BYTES) == 0);
    final = entry_cpu();
    CHECK(memcmp(&cpu, &final, sizeof(cpu)) == 0);
    free(before);
    free(ram);
    return 0;
}

static int load_capture(const char *path, uint8_t *ram)
{
    FILE *fp = fopen(path, "rb");
    size_t count;
    int extra;
    if (fp == NULL) return 0;
    count = fread(ram, 1u, CAPTURE_BYTES, fp);
    extra = fgetc(fp);
    if (fclose(fp) != 0) return 0;
    return count == CAPTURE_BYTES && extra == EOF;
}

static int native_pair(const char *entry_path, const char *return_path)
{
    uint8_t *ram = calloc(PR_R464_2632_RAM_BYTES, 1u);
    uint8_t *native = malloc(CAPTURE_BYTES);
    pr_r464_2632_cpu cpu = entry_cpu();
    pr_r464_2632_cpu final = return_cpu();
    CHECK(ram != NULL && native != NULL);
    CHECK(load_capture(entry_path, ram));
    CHECK(load_capture(return_path, native));
    CHECK(pr_r464_2632_run014(ram, PR_R464_2632_RAM_BYTES,
                               &cpu, NULL, NULL) == PR_R464_2632_OK);
    CHECK(memcmp(ram, native, CAPTURE_BYTES) == 0);
    CHECK(memcmp(&cpu, &final, sizeof(cpu)) == 0);
    printf("native RAM %u/%u and CPU 14/14 exact\n",
           CAPTURE_BYTES, CAPTURE_BYTES);
    free(native);
    free(ram);
    return 0;
}

int main(int argc, char **argv)
{
    if (synthetic() != 0) return 1;
    if (argc == 1) return 0;
    if (argc != 3) return 2;
    return native_pair(argv[1], argv[2]);
}
