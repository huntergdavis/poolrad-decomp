#include "poolrad/recovered/overlay_u40c_tile_plane.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define HEADER 0x17u
#define DEST_BYTES (21u * 168u * 4u)

static void put_word(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

int main(void)
{
    uint8_t source[HEADER + 70u * 32u] = {0};
    uint8_t mask[70u * 8u];
    uint8_t destination[HEADER + DEST_BYTES] = {0};
    uint8_t snapshot[sizeof destination];
    pr_u40c_tile_plane_state state = {0};
    pr_u40c_tile_plane_report report;
    size_t second_offset = (7u * 8u * 21u + 7u) * 4u;

    put_word(source, 8u);
    put_word(source + 2u, 1u);
    put_word(source + 0x11u, 32u);
    put_word(source + 0x13u, 1u); /* nonnull auxiliary far pointer */
    put_word(destination, 168u);
    put_word(destination + 2u, 21u);
    memset(destination + HEADER, 0x33, DEST_BYTES);
    memset(mask, 0xff, sizeof mask);

    state.item_id = 1;
    state.group0_remap = 1;
    state.x = 7;
    state.y = 6;
    state.flag = 1;
    state.video_state = 1;
    state.source = source;
    state.source_size = sizeof source;
    state.aux_mask = mask;
    state.aux_mask_size = sizeof mask;
    state.destination = destination;
    state.destination_size = sizeof destination;

    /* Synthetic item 1 has zero source bytes and all-one mask, so every
     * reached write preserves the destination byte. */
    assert(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
           PR_U40C_TILE_PLANE_OK);
    assert(report.item_index == 0 && report.first_data_offset == 4060);
    assert(report.bytes_written == 32 && report.bytes_changed == 0);
    assert(report.first_changed_offset == (size_t)-1);
    assert(destination[HEADER + 4060] == 0x33);

    /* Synthetic item 13 exercises OR and AND on selected EGA plane bytes;
     * the row increment is 21*4, while the item stride is 32. */
    state.item_id = 13;
    state.y = 7;
    source[HEADER + 12u * 32u + 2u] = 0x08;
    source[HEADER + 12u * 32u + 4u] = 0x05;
    mask[12u * 8u + 1u] = 0xf0;
    assert(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
           PR_U40C_TILE_PLANE_OK);
    assert(report.item_index == 12 && report.first_data_offset == second_offset);
    assert(report.bytes_written == 32 && report.bytes_changed == 5);
    assert(report.first_changed_offset == second_offset + 2u);
    assert(destination[HEADER + second_offset] == 0x33);
    assert(destination[HEADER + second_offset + 2u] == 0x3b);
    assert(destination[HEADER + second_offset + 4u] == 0x33);
    assert(destination[HEADER + second_offset + 84u] == 0x35);
    assert(destination[HEADER + second_offset + 85u] == 0x30);
    assert(destination[HEADER + second_offset - 1u] == 0x33);

    memcpy(snapshot, destination, sizeof snapshot);
    state.item_id = 46; /* another group has a different pointer/remap */
    assert(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
           PR_U40C_TILE_PLANE_OTHER_PATH);
    state.item_id = 13;
    state.flag = 0; /* entry 2 takes its separate +0x0250 branch */
    assert(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
           PR_U40C_TILE_PLANE_OTHER_PATH);
    state.flag = 1;
    state.video_state = 2;
    assert(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
           PR_U40C_TILE_PLANE_OTHER_PATH);
    state.video_state = 1;
    state.x = 21;
    assert(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
           PR_U40C_TILE_PLANE_OTHER_PATH);
    state.x = 7;
    state.y = 21;
    assert(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
           PR_U40C_TILE_PLANE_OTHER_PATH);
    state.y = 7;
    state.aux_mask_size = 103; /* item index 12 needs 104 bytes */
    assert(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
           PR_U40C_TILE_PLANE_BOUNDS);
    state.aux_mask_size = sizeof mask;
    state.destination_size = HEADER + second_offset + 31u;
    assert(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
           PR_U40C_TILE_PLANE_BOUNDS);
    state.destination_size = sizeof destination;
    put_word(source + 0x13u, 0u);
    assert(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
           PR_U40C_TILE_PLANE_OTHER_PATH);
    assert(memcmp(destination, snapshot, sizeof snapshot) == 0);
    assert(pr_u40c_entry2_mode1_tile_plane(NULL, &report) ==
           PR_U40C_TILE_PLANE_ARGUMENT);

    puts("unit 0x40c saved tile plane-byte slice tests passed");
    return 0;
}
