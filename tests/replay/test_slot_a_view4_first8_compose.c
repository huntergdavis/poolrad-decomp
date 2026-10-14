#include "poolrad/recovered/overlay_u40c_tile_plane.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    HEADER = 0x17,
    SOURCE_BYTES = HEADER + 45 * 32,
    MASK_BYTES = 45 * 8,
    DEST_BYTES = HEADER + 168 * 21 * 4
};

typedef struct {
    uint8_t x, y;
    uint16_t item_id;
    unsigned changed;
} call;

static void put_word(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

int main(void)
{
    static const call calls[8] = {
        {4u, 3u, 1u, 0u}, {5u, 3u, 1u, 0u},
        {4u, 4u, 18u, 16u}, {5u, 4u, 1u, 0u},
        {4u, 5u, 13u, 8u}, {5u, 5u, 18u, 16u},
        {4u, 6u, 13u, 8u}, {5u, 6u, 13u, 8u}
    };
    uint8_t source[SOURCE_BYTES] = {0};
    uint8_t mask[MASK_BYTES];
    uint8_t destination[DEST_BYTES] = {0};
    uint8_t expected[DEST_BYTES];
    pr_u40c_tile_plane_state state = {0};
    pr_u40c_tile_plane_report report;
    unsigned row, plane, step;

    put_word(source, 8u);
    put_word(source + 2u, 1u);
    put_word(source + 0x11u, 32u);
    put_word(source + 0x13u, 1u); /* Synthetic nonnull auxiliary pointer. */
    put_word(destination, 168u);
    put_word(destination + 2u, 21u);
    memset(destination + HEADER, 0x33, DEST_BYTES - HEADER);
    memcpy(expected, destination, sizeof expected);
    memset(mask, 0xff, sizeof mask);

    /* Index 0 leaves the object intact. Index 17 clears the last two
     * planes; index 12 clears only the last plane. These fixtures make
     * retained effects from earlier calls visible in whole-object checks. */
    for (row = 0; row < 8u; ++row) {
        mask[17u * 8u + row] = 0xfeu;
        mask[12u * 8u + row] = 0xfeu;
        for (plane = 0; plane < 2u; ++plane)
            source[HEADER + 17u * 32u + row * 4u + plane] = 1u;
        for (plane = 0; plane < 3u; ++plane)
            source[HEADER + 12u * 32u + row * 4u + plane] = 1u;
    }

    state.group0_remap = 1u;
    state.flag = 1u;
    state.video_state = 1u;
    state.source = source;
    state.source_size = sizeof source;
    state.aux_mask = mask;
    state.aux_mask_size = sizeof mask;
    state.destination = destination;
    state.destination_size = sizeof destination;

    for (step = 0; step < 8u; ++step) {
        size_t first_data = ((size_t)calls[step].y * 8u * 21u +
                             calls[step].x) * 4u;
        unsigned first_changed_plane = calls[step].item_id == 18u ? 2u : 3u;

        state.x = calls[step].x;
        state.y = calls[step].y;
        state.item_id = calls[step].item_id;
        assert(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
               PR_U40C_TILE_PLANE_OK);
        assert(report.item_index == calls[step].item_id - 1u);
        assert(report.first_data_offset == first_data);
        assert(report.bytes_written == 32u);
        assert(report.bytes_changed == calls[step].changed);
        if (calls[step].changed == 0u) {
            assert(report.first_changed_offset == (size_t)-1);
        } else {
            assert(report.first_changed_offset ==
                   first_data + first_changed_plane);
            for (row = 0; row < 8u; ++row) {
                size_t at = HEADER + first_data + row * 21u * 4u;
                for (plane = first_changed_plane; plane < 4u; ++plane)
                    expected[at + plane] = 0x32u;
            }
        }
        assert(memcmp(destination, expected, sizeof expected) == 0);
    }

    puts("saved view-four first-eight persistent-object composition passed");
    return 0;
}
