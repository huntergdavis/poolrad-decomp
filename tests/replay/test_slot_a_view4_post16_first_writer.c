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
    WRITER_INDEX = 39,
    WRITER_DATA_OFFSET = (6 * 8 * 21 + 12) * 4
};

static void put_word(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
}

int main(void)
{
    uint8_t source[SOURCE_BYTES] = {0};
    uint8_t mask[MASK_BYTES];
    uint8_t destination[DEST_BYTES], expected[DEST_BYTES];
    pr_u40c_tile_plane_state state = {0};
    pr_u40c_tile_plane_report report;
    unsigned row;

    put_word(source, 8u);
    put_word(source + 2u, 1u);
    put_word(source + 0x11u, 32u);
    put_word(source + 0x13u, 1u);
    put_word(destination, 168u);
    put_word(destination + 2u, 21u);
    memset(destination + HEADER, 0x33, DEST_BYTES - HEADER);
    memcpy(expected, destination, sizeof expected);
    memset(mask, 0xff, sizeof mask);
    for (row = 0; row < 8u; ++row) {
        size_t source_row = HEADER + WRITER_INDEX * 32u + row * 4u;
        mask[WRITER_INDEX * 8u + row] = 0xfeu;
        source[source_row] = 1u;
        source[source_row + 1u] = 1u;
    }

    state.group0_remap = 1u;
    state.flag = state.video_state = 1u;
    state.source = source;
    state.source_size = sizeof source;
    state.aux_mask = mask;
    state.aux_mask_size = sizeof mask;
    state.destination = destination;
    state.destination_size = sizeof destination;

    /* First reached call after the 16-tile view-four placement. */
    state.item_id = 1u;
    state.x = 12u;
    state.y = 5u;
    assert(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
           PR_U40C_TILE_PLANE_OK);
    assert(report.item_index == 0u && report.bytes_written == 32u);
    assert(report.bytes_changed == 0u &&
           report.first_changed_offset == (size_t)-1);
    assert(memcmp(destination, expected, sizeof expected) == 0);

    /* Second reached call uses the high group-zero index. Reject a short
     * host source before any destination write, then apply all 32 stores. */
    state.item_id = 40u;
    state.y = 6u;
    state.source_size = HEADER + 40u * 32u - 1u;
    assert(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
           PR_U40C_TILE_PLANE_BOUNDS);
    assert(memcmp(destination, expected, sizeof expected) == 0);
    state.source_size = sizeof source;
    assert(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
           PR_U40C_TILE_PLANE_OK);
    assert(report.item_index == WRITER_INDEX);
    assert(report.first_data_offset == WRITER_DATA_OFFSET);
    assert(report.first_changed_offset == WRITER_DATA_OFFSET + 2u);
    assert(report.bytes_written == 32u && report.bytes_changed == 16u);
    for (row = 0; row < 8u; ++row) {
        size_t at = HEADER + WRITER_DATA_OFFSET + row * 21u * 4u;
        expected[at + 2u] = 0x32u;
        expected[at + 3u] = 0x32u;
    }
    assert(memcmp(destination, expected, sizeof expected) == 0);

    puts("post-view-four first writer sequence passed");
    return 0;
}
