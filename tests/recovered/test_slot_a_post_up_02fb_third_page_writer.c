#include "poolrad/recovered/overlay_u40c_tile_plane.h"

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
    RAM_BYTES = 0xA0000u,
    OBJECT_BYTES = 14135u,
    HEADER_BYTES = 23u,
    DS_BASE = 0x14A00u,
    SS_BASE = 0x1D160u,
    CALL_SP = 0x3EA2u
};

static uint16_t word_at(const uint8_t *p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static void put_word(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static int synthetic(void)
{
    uint8_t source[HEADER_BYTES + 45u * 32u] = {0};
    uint8_t mask[45u * 8u];
    uint8_t destination[OBJECT_BYTES] = {0};
    uint8_t before[OBJECT_BYTES];
    pr_u40c_tile_plane_state state = {0};
    pr_u40c_tile_plane_report report;
    unsigned row;

    put_word(source, 8u);
    put_word(source + 2u, 1u);
    put_word(source + 0x11u, 32u);
    put_word(source + 0x13u, 1u);
    put_word(destination, 168u);
    put_word(destination + 2u, 21u);
    memset(mask, 0xFF, sizeof mask);
    for (row = 0u; row < 8u; ++row) {
        size_t data = HEADER_BYTES + 4728u + row * 84u;
        destination[data + 3u] = 0xFFu;
        if (row < 7u) {
            size_t item = HEADER_BYTES + 38u * 32u + row * 4u;
            mask[38u * 8u + row] = 0u;
            source[item + 2u] = 0xFEu;
        }
    }
    state.item_id = 39u;
    state.group0_remap = 1u;
    state.x = 6u;
    state.y = 7u;
    state.flag = 1u;
    state.video_state = 1u;
    state.source = source;
    state.source_size = sizeof source;
    state.aux_mask = mask;
    state.aux_mask_size = sizeof mask;
    state.destination = destination;
    state.destination_size = sizeof destination;
    memcpy(before, destination, sizeof before);
    CHECK(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
          PR_U40C_TILE_PLANE_OK);
    CHECK(report.item_index == 38u && report.first_data_offset == 4728u);
    CHECK(report.first_changed_offset == 4730u);
    CHECK(report.bytes_written == 32u && report.bytes_changed == 14u);
    for (size_t i = HEADER_BYTES; i < sizeof destination; ++i) {
        size_t data = i - HEADER_BYTES;
        unsigned row_number = (unsigned)(data / 84u);
        unsigned col = (unsigned)((data % 84u) / 4u);
        unsigned plane = (unsigned)(data % 4u);
        uint8_t expected = before[i];
        if (row_number >= 56u && row_number < 63u && col == 6u)
            expected = plane == 2u ? 0xFEu : plane == 3u ? 0u : expected;
        CHECK(destination[i] == expected);
    }
    return 0;
}

static int load_ram(const char *path, uint8_t *ram)
{
    FILE *fp = fopen(path, "rb");
    size_t count;
    int extra;
    if (fp == NULL) return 0;
    count = fread(ram, 1u, RAM_BYTES, fp);
    extra = fgetc(fp);
    if (fclose(fp) != 0) return 0;
    return count == RAM_BYTES && extra == EOF;
}

static int private_pair(const char *pre_path, const char *return_path)
{
    uint8_t *pre = malloc(RAM_BYTES);
    uint8_t *native = malloc(RAM_BYTES);
    pr_u40c_tile_plane_state state = {0};
    pr_u40c_tile_plane_report report;
    size_t stack, source, aux, dest;
    uint16_t source_off, source_seg, aux_off, aux_seg, dest_off, dest_seg;
    CHECK(pre != NULL && native != NULL);
    CHECK(load_ram(pre_path, pre) && load_ram(return_path, native));
    stack = SS_BASE + CALL_SP;
    CHECK(word_at(pre + stack) == 1u &&
          word_at(pre + stack + 2u) == 1u &&
          word_at(pre + stack + 4u) == 39u &&
          word_at(pre + stack + 6u) == 7u &&
          word_at(pre + stack + 8u) == 6u);
    source_off = word_at(pre + DS_BASE + 0x5E3Au);
    source_seg = word_at(pre + DS_BASE + 0x5E3Cu);
    source = ((size_t)source_seg << 4) + source_off;
    CHECK(source + HEADER_BYTES <= RAM_BYTES);
    aux_off = word_at(pre + source + 0x13u);
    aux_seg = word_at(pre + source + 0x15u);
    aux = ((size_t)aux_seg << 4) + aux_off;
    dest_off = word_at(pre + DS_BASE + 0x4A20u);
    dest_seg = word_at(pre + DS_BASE + 0x4A22u);
    dest = ((size_t)dest_seg << 4) + dest_off;
    CHECK(aux < RAM_BYTES && dest + OBJECT_BYTES <= RAM_BYTES);
    state.item_id = word_at(pre + stack + 4u);
    state.group0_remap = word_at(pre + DS_BASE + 0x2722u);
    state.x = (uint8_t)word_at(pre + stack + 8u);
    state.y = (uint8_t)word_at(pre + stack + 6u);
    state.flag = (uint8_t)word_at(pre + stack + 2u);
    state.video_state = pre[DS_BASE + 0x4A1Eu];
    state.source = pre + source;
    state.source_size = RAM_BYTES - source;
    state.aux_mask = pre + aux;
    state.aux_mask_size = RAM_BYTES - aux;
    state.destination = pre + dest;
    state.destination_size = RAM_BYTES - dest;
    CHECK(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
          PR_U40C_TILE_PLANE_OK);
    CHECK(report.item_index == 38u && report.first_data_offset == 4728u);
    CHECK(report.first_changed_offset == 4730u);
    CHECK(report.bytes_written == 32u && report.bytes_changed == 14u);
    CHECK(memcmp(pre + dest, native + dest, OBJECT_BYTES) == 0);
    CHECK(memcmp(pre + DS_BASE + 0x4A28u,
                 native + DS_BASE + 0x4A28u, 0x4EC0u - 0x4A28u) == 0);
    puts("third call: object 14135/14135 exact, metadata 1176/1176 exact; "
         "writes 32, changed 14");
    free(native);
    free(pre);
    return 0;
}

int main(int argc, char **argv)
{
    if (synthetic() != 0) return 1;
    if (argc == 1) return 0;
    if (argc != 3) return 2;
    return private_pair(argv[1], argv[2]);
}
