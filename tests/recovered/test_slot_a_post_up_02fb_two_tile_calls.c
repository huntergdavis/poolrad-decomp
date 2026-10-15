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
    DS = 0x14A00u,
    SS = 0x1D160u,
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
    for (row = 1u; row < 8u; ++row) {
        source[HEADER_BYTES + 17u * 32u + row * 4u + 2u] = 0x80u;
        source[HEADER_BYTES + 17u * 32u + row * 4u + 3u] = 0x40u;
    }
    state.item_id = 1u;
    state.group0_remap = 1u;
    state.x = 6u;
    state.y = 5u;
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
    CHECK(report.item_index == 0u && report.first_data_offset == 3384u);
    CHECK(report.bytes_written == 32u && report.bytes_changed == 0u);
    CHECK(memcmp(destination, before, sizeof before) == 0);

    state.item_id = 18u;
    state.y = 6u;
    CHECK(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
          PR_U40C_TILE_PLANE_OK);
    CHECK(report.item_index == 17u && report.first_data_offset == 4056u);
    CHECK(report.bytes_written == 32u && report.bytes_changed == 14u);
    CHECK(report.first_changed_offset == 4142u);
    for (size_t i = HEADER_BYTES; i < sizeof destination; ++i) {
        size_t data = i - HEADER_BYTES;
        unsigned r = (unsigned)(data / 84u);
        unsigned col = (unsigned)((data % 84u) / 4u);
        unsigned plane = (unsigned)(data % 4u);
        uint8_t expected = 0u;
        if (r >= 49u && r <= 55u && col == 6u)
            expected = plane == 2u ? 0x80u : plane == 3u ? 0x40u : 0u;
        CHECK(destination[i] == expected);
    }
    memcpy(before, destination, sizeof before);
    state.video_state = 2u;
    CHECK(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
          PR_U40C_TILE_PLANE_OTHER_PATH);
    CHECK(memcmp(destination, before, sizeof before) == 0);
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

static int private_pair(const char *pre_path, const char *ret_path,
                        unsigned ordinal)
{
    uint8_t *pre = malloc(RAM_BYTES);
    uint8_t *ret = malloc(RAM_BYTES);
    pr_u40c_tile_plane_state state = {0};
    pr_u40c_tile_plane_report report;
    uint16_t source_off, source_seg, aux_off, aux_seg, dest_off, dest_seg;
    size_t source, aux, dest, stack, changed = 0u;
    unsigned expected_item = ordinal == 1u ? 1u : 18u;
    unsigned expected_y = ordinal == 1u ? 5u : 6u;
    CHECK(pre != NULL && ret != NULL);
    CHECK(load_ram(pre_path, pre) && load_ram(ret_path, ret));
    stack = SS + CALL_SP;
    CHECK(word_at(pre + stack) == 1u &&
          word_at(pre + stack + 2u) == 1u &&
          word_at(pre + stack + 4u) == expected_item &&
          word_at(pre + stack + 6u) == expected_y &&
          word_at(pre + stack + 8u) == 6u);
    source_off = word_at(pre + DS + 0x5E3Au);
    source_seg = word_at(pre + DS + 0x5E3Cu);
    source = ((size_t)source_seg << 4) + source_off;
    CHECK(source + HEADER_BYTES <= RAM_BYTES);
    aux_off = word_at(pre + source + 0x13u);
    aux_seg = word_at(pre + source + 0x15u);
    aux = ((size_t)aux_seg << 4) + aux_off;
    dest_off = word_at(pre + DS + 0x4A20u);
    dest_seg = word_at(pre + DS + 0x4A22u);
    dest = ((size_t)dest_seg << 4) + dest_off;
    CHECK(aux < RAM_BYTES && dest + OBJECT_BYTES <= RAM_BYTES);
    state.item_id = word_at(pre + stack + 4u);
    state.group0_remap = word_at(pre + DS + 0x2722u);
    state.x = (uint8_t)word_at(pre + stack + 8u);
    state.y = (uint8_t)word_at(pre + stack + 6u);
    state.flag = (uint8_t)word_at(pre + stack + 2u);
    state.video_state = pre[DS + 0x4A1Eu];
    state.source = pre + source;
    state.source_size = RAM_BYTES - source;
    state.aux_mask = pre + aux;
    state.aux_mask_size = RAM_BYTES - aux;
    state.destination = pre + dest;
    state.destination_size = RAM_BYTES - dest;
    for (size_t i = 0u; i < OBJECT_BYTES; ++i)
        if (pre[dest + i] != ret[dest + i]) ++changed;
    CHECK(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
          PR_U40C_TILE_PLANE_OK);
    CHECK(report.item_index == (ordinal == 1u ? 0u : 17u));
    CHECK(report.first_data_offset == (ordinal == 1u ? 3384u : 4056u));
    CHECK(report.bytes_written == 32u && report.bytes_changed == changed);
    CHECK(changed == (ordinal == 1u ? 0u : 14u));
    CHECK(memcmp(pre + dest, ret + dest, OBJECT_BYTES) == 0);
    CHECK(memcmp(pre + DS + 0x4A28u, ret + DS + 0x4A28u,
                 0x4EC0u - 0x4A28u) == 0);
    printf("call %u: object %u/%u exact, writes %u, changed %zu, "
           "metadata %u/%u exact\n", ordinal, OBJECT_BYTES, OBJECT_BYTES,
           report.bytes_written, changed, 0x4EC0u - 0x4A28u,
           0x4EC0u - 0x4A28u);
    free(ret);
    free(pre);
    return 0;
}

int main(int argc, char **argv)
{
    if (synthetic() != 0) return 1;
    if (argc == 1) return 0;
    if (argc != 5) return 2;
    if (private_pair(argv[1], argv[2], 1u) != 0) return 1;
    return private_pair(argv[3], argv[4], 2u);
}
