#include "poolrad/recovered/overlay_u40c_tile_plane.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    HEADER = 0x17,
    SOURCE_BYTES = HEADER + 45 * 32,
    MASK_BYTES = 45 * 8,
    DEST_BYTES = HEADER + 168 * 21 * 4,
    ITEM_INDEX = 12,
    FIFTH_DATA_OFFSET = (5 * 8 * 21 + 4) * 4
};

static void put_word(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

int main(void)
{
    uint8_t source[SOURCE_BYTES] = {0};
    uint8_t mask[MASK_BYTES];
    uint8_t destination[DEST_BYTES] = {0};
    uint8_t expected[DEST_BYTES];
    pr_u40c_tile_plane_state state = {0};
    pr_u40c_tile_plane_report report;
    unsigned row;

    put_word(source, 8u);
    put_word(source + 2u, 1u);
    put_word(source + 0x11u, 32u);
    put_word(source + 0x13u, 1u); /* synthetic nonnull auxiliary pointer */
    put_word(destination, 168u);
    put_word(destination + 2u, 21u);
    memset(destination + HEADER, 0x33, DEST_BYTES - HEADER);
    memset(mask, 0xff, sizeof mask);

    /* At every row, AND clears bit zero. Source restores it in planes 0/1;
     * only planes 2/3 change. These are synthetic bytes, not game art. */
    for (row = 0; row < 8u; ++row) {
        mask[ITEM_INDEX * 8u + row] = 0xfeu;
        source[HEADER + ITEM_INDEX * 32u + row * 4u] = 1u;
        source[HEADER + ITEM_INDEX * 32u + row * 4u + 1u] = 1u;
    }
    memcpy(expected, destination, sizeof expected);
    for (row = 0; row < 8u; ++row) {
        size_t at = HEADER + FIFTH_DATA_OFFSET + row * 21u * 4u;
        expected[at + 2u] = 0x32u;
        expected[at + 3u] = 0x32u;
    }

    state.item_id = 13u;
    state.group0_remap = 1u;
    state.x = 4u;
    state.y = 5u;
    state.flag = 1u;
    state.video_state = 1u;
    state.source = source;
    state.source_size = sizeof source;
    state.aux_mask = mask;
    state.aux_mask_size = sizeof mask;
    state.destination = destination;
    state.destination_size = sizeof destination;

    assert(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
           PR_U40C_TILE_PLANE_OK);
    assert(report.item_index == ITEM_INDEX);
    assert(report.first_data_offset == FIFTH_DATA_OFFSET);
    assert(report.first_changed_offset == FIFTH_DATA_OFFSET + 2u);
    assert(report.bytes_written == 32u && report.bytes_changed == 16u);
    assert(memcmp(destination, expected, sizeof expected) == 0);

    /* Index 12 needs 104 auxiliary bytes; reject one byte less before any
     * destination write, including the first reached plane byte. */
    memset(destination + HEADER, 0x33, DEST_BYTES - HEADER);
    memcpy(expected, destination, sizeof expected);
    state.aux_mask_size = ITEM_INDEX * 8u + 7u;
    assert(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
           PR_U40C_TILE_PLANE_BOUNDS);
    assert(memcmp(destination, expected, sizeof expected) == 0);

    puts("saved view-four fifth tile plane-byte tests passed");
    return 0;
}
