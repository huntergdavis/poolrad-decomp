#include "poolrad/recovered/resident_464_2632_post_up_43row.h"

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
    RAM_BYTES = PR_R464_2632_POST_UP_RAM_BYTES,
    DS = 0x14A00u,
    SS = 0x1D160u,
    OBJECT = 0x3DE48u,
    OBJECT_BYTES = 14135u
};

static const pr_r464_2632_post_up_frame frame = {
    0x2D14u, 0x0198u, 0x1D16u, 0x3EB0u, 0x14A0u
};

static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[at + 1u] << 8));
}

static void put_word(uint8_t *ram, size_t at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[at + 1u] = (uint8_t)(value >> 8);
}

static void setup(uint8_t *ram)
{
    unsigned row;
    size_t stack = SS + frame.sp;
    memset(ram, 0, RAM_BYTES);
    memset(ram + OBJECT + 23u, 0xA5, OBJECT_BYTES - 23u);
    put_word(ram, OBJECT, 168u);
    put_word(ram, OBJECT + 2u, 21u);
    put_word(ram, stack, 0x0008u);
    put_word(ram, stack + 2u, 0x3DE4u);
    put_word(ram, stack + 4u, 0x260Au);
    put_word(ram, stack + 6u, 0x14A0u);
    put_word(ram, stack + 8u, 0x000Bu);
    put_word(ram, stack + 10u, 43u);
    put_word(ram, stack + 12u, 11u);
    put_word(ram, stack + 14u, 16u);
    put_word(ram, stack + 16u, 2u);
    ram[DS + 0x4A1Eu] = 1u;
    ram[DS + 0x25F6u] = 0x08u;
    ram[DS + 0x25F7u] = 0x10u;
    ram[DS + 0x25F8u] = 0x03u;
    ram[DS + 0x25F9u] = 0x01u;
    for (row = 0u; row < 168u; ++row) {
        put_word(ram, DS + 0x4C20u + 2u * row, 0xFFFFu);
    }
}

static int synthetic(void)
{
    uint8_t *ram = malloc(RAM_BYTES);
    uint8_t *before = malloc(RAM_BYTES);
    pr_r464_2632_post_up_result result = {0};
    unsigned row, column;
    size_t stack = SS + frame.sp;
    CHECK(ram != NULL && before != NULL);
    setup(ram);
    memcpy(before, ram, RAM_BYTES);
    CHECK(pr_r464_2632_post_up_43row(ram, RAM_BYTES, &frame, &result) ==
          PR_R464_2632_POST_UP_OK);
    CHECK(result.first_row == 16u && result.last_row == 58u);
    CHECK(result.offscreen_value_changes == 43u * 11u * 4u);
    CHECK(memcmp(ram + OBJECT, before + OBJECT, 23u) == 0);
    CHECK(ram[DS + 0x70C7u] == 11u);
    for (row = 0u; row < 168u; ++row) {
        size_t ext = DS + 2u * row;
        if (row >= 16u && row <= 58u) {
            uint16_t start = (uint16_t)((row * 21u + 2u) * 4u);
            CHECK(ram[DS + 0x4A28u + row] == 1u);
            CHECK(word_at(ram, ext + 0x4AD0u) == 2u);
            CHECK(word_at(ram, ext + 0x4C20u) == start);
            CHECK(word_at(ram, ext + 0x4D70u) == start + 43u);
            for (column = 0u; column < 21u; ++column) {
                size_t at = OBJECT + 23u + (row * 21u + column) * 4u;
                if (column >= 2u && column <= 12u) {
                    CHECK(ram[at] == 0xFFu && ram[at + 1u] == 0u);
                    CHECK(ram[at + 2u] == 0xFFu && ram[at + 3u] == 0xFFu);
                } else {
                    CHECK(memcmp(ram + at, before + at, 4u) == 0);
                }
            }
        } else {
            CHECK(ram[DS + 0x4A28u + row] == 0u);
            CHECK(word_at(ram, ext + 0x4C20u) == 0xFFFFu);
        }
    }
    CHECK(memcmp(ram + OBJECT + 23u + 15u * 21u * 4u,
                 before + OBJECT + 23u + 15u * 21u * 4u,
                 21u * 4u) == 0);
    CHECK(memcmp(ram + OBJECT + 23u + 59u * 21u * 4u,
                 before + OBJECT + 23u + 59u * 21u * 4u,
                 21u * 4u) == 0);

    /* A preexisting wider extent is retained while the cell bytes update. */
    setup(ram);
    put_word(ram, DS + 0x4AD0u + 40u, 7u);
    put_word(ram, DS + 0x4C20u + 40u, 1u);
    put_word(ram, DS + 0x4D70u + 40u, 0xFFFFu);
    CHECK(pr_r464_2632_post_up_43row(ram, RAM_BYTES, &frame, &result) ==
          PR_R464_2632_POST_UP_OK);
    CHECK(word_at(ram, DS + 0x4AD0u + 40u) == 7u);
    CHECK(word_at(ram, DS + 0x4C20u + 40u) == 1u);
    CHECK(word_at(ram, DS + 0x4D70u + 40u) == 0xFFFFu);

    setup(ram);
    memcpy(before, ram, RAM_BYTES);
    put_word(ram, stack + 10u, 42u);
    memcpy(before, ram, RAM_BYTES);
    CHECK(pr_r464_2632_post_up_43row(ram, RAM_BYTES, &frame, &result) ==
          PR_R464_2632_POST_UP_BOUNDARY);
    CHECK(memcmp(ram, before, RAM_BYTES) == 0);
    setup(ram);
    ram[DS + 0x4A1Eu] = 0u;
    memcpy(before, ram, RAM_BYTES);
    CHECK(pr_r464_2632_post_up_43row(ram, RAM_BYTES, &frame, &result) ==
          PR_R464_2632_POST_UP_BOUNDARY);
    CHECK(memcmp(ram, before, RAM_BYTES) == 0);
    CHECK(pr_r464_2632_post_up_43row(NULL, RAM_BYTES, &frame, &result) ==
          PR_R464_2632_POST_UP_ARGUMENT);
    CHECK(pr_r464_2632_post_up_43row(ram, RAM_BYTES, NULL, &result) ==
          PR_R464_2632_POST_UP_ARGUMENT);
    free(before);
    free(ram);
    return 0;
}

static int load_ram(const char *path, uint8_t *ram)
{
    FILE *fp = fopen(path, "rb");
    int extra;
    if (fp == NULL) return 0;
    if (fread(ram, 1u, RAM_BYTES, fp) != RAM_BYTES) {
        fclose(fp);
        return 0;
    }
    extra = fgetc(fp);
    if (fclose(fp) != 0) return 0;
    return extra == EOF;
}

static int private_pair(const char *entry_path, const char *return_path)
{
    uint8_t *entry = malloc(RAM_BYTES);
    uint8_t *native = malloc(RAM_BYTES);
    pr_r464_2632_post_up_result result = {0};
    size_t native_changes = 0u;
    size_t stack_residual = 0u;
    size_t i;
    CHECK(entry != NULL && native != NULL);
    CHECK(load_ram(entry_path, entry));
    CHECK(load_ram(return_path, native));
    for (i = 0u; i < OBJECT_BYTES; ++i)
        if (entry[OBJECT + i] != native[OBJECT + i]) ++native_changes;
    CHECK(pr_r464_2632_post_up_43row(entry, RAM_BYTES, &frame, &result) ==
          PR_R464_2632_POST_UP_OK);
    CHECK(result.first_row == 16u && result.last_row == 58u);
    CHECK(result.offscreen_value_changes == native_changes);
    CHECK(memcmp(entry + OBJECT, native + OBJECT, OBJECT_BYTES) == 0);
    CHECK(memcmp(entry + DS + 0x4A28u, native + DS + 0x4A28u,
                 0x4EC0u - 0x4A28u) == 0);
    CHECK(entry[DS + 0x70C7u] == native[DS + 0x70C7u]);
    for (i = 0u; i < RAM_BYTES; ++i) {
        if (entry[i] == native[i]) continue;
        CHECK(i >= SS + 0x3E60u && i < SS + 0x3EC2u);
        ++stack_residual;
    }
    printf("native object %u/%u, dirty and extent metadata %u/%u, "
           "loop counter 1/1 exact; changed object bytes %zu; "
           "stack-only residual %zu\n",
           OBJECT_BYTES, OBJECT_BYTES, 0x4EC0u - 0x4A28u,
           0x4EC0u - 0x4A28u, result.offscreen_value_changes,
           stack_residual);
    free(native);
    free(entry);
    return 0;
}

int main(int argc, char **argv)
{
    if (synthetic() != 0) return 1;
    if (argc == 1) return 0;
    if (argc != 3) return 2;
    return private_pair(argv[1], argv[2]);
}
