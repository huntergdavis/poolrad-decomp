#include "poolrad/recovered/overlay_u40c_group3_tile_plane.h"
#include "poolrad/recovered/overlay_u40c_tile_plane.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    HEADER = 0x17,
    GROUP0_SOURCE_BYTES = HEADER + 45 * 32,
    GROUP3_SOURCE_BYTES = HEADER + 51 * 32,
    GROUP0_MASK_BYTES = 45 * 8,
    GROUP3_MASK_BYTES = 51 * 8,
    DEST_BYTES = HEADER + 168 * 21 * 4
};

typedef struct {
    uint8_t x, y;
    uint16_t item, index;
    uint8_t group, first_plane;
} tile_call;

static void put_word(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
}

static void init_source(uint8_t *source, size_t source_size,
                        uint8_t *mask, size_t mask_size)
{
    memset(source, 0, source_size);
    memset(mask, 0xff, mask_size);
    put_word(source, 8u);
    put_word(source + 2u, 1u);
    put_word(source + 0x11u, 32u);
    put_word(source + 0x13u, 1u);
}

int main(void)
{
    static const tile_call calls[] = {
        {4u, 8u, 228u, 42u, 3u, 0u},
        {5u, 8u, 236u, 50u, 3u, 1u},
        {4u, 9u, 228u, 42u, 3u, 0u},
        {5u, 9u, 23u, 22u, 0u, 1u},
        {4u, 10u, 21u, 20u, 0u, 2u},
        {5u, 10u, 3u, 2u, 0u, 4u}
    };
    uint8_t group0_source[GROUP0_SOURCE_BYTES];
    uint8_t group3_source[GROUP3_SOURCE_BYTES];
    uint8_t group0_mask[GROUP0_MASK_BYTES];
    uint8_t group3_mask[GROUP3_MASK_BYTES];
    uint8_t destination[DEST_BYTES], expected[DEST_BYTES];
    unsigned step, row, plane;

    init_source(group0_source, sizeof group0_source,
                group0_mask, sizeof group0_mask);
    init_source(group3_source, sizeof group3_source,
                group3_mask, sizeof group3_mask);
    put_word(destination, 168u);
    put_word(destination + 2u, 21u);
    memset(destination + HEADER, 0x33, DEST_BYTES - HEADER);
    memcpy(expected, destination, sizeof expected);
    for (row = 0; row < 8u; ++row) {
        group3_mask[42u * 8u + row] = 0xfeu;
        group3_mask[50u * 8u + row] = 0xfeu;
        group0_mask[22u * 8u + row] = 0xfeu;
        group0_mask[20u * 8u + row] = 0xfeu;
        group3_source[HEADER + 50u * 32u + row * 4u] = 1u;
        group0_source[HEADER + 22u * 32u + row * 4u] = 1u;
        group0_source[HEADER + 20u * 32u + row * 4u] = 1u;
        group0_source[HEADER + 20u * 32u + row * 4u + 1u] = 1u;
    }

    for (step = 0; step < sizeof calls / sizeof calls[0]; ++step) {
        const tile_call *call = &calls[step];
        size_t first = ((size_t)call->y * 8u * 21u + call->x) * 4u;
        unsigned changed = 8u * (4u - call->first_plane);

        if (call->group == 3u) {
            pr_u40c_group3_tile_plane_state state = {0};
            pr_u40c_group3_tile_plane_report report;
            state.item_id = call->item;
            state.group3_remap = 186u;
            state.x = call->x;
            state.y = call->y;
            state.flag = state.video_state = 1u;
            state.source = group3_source;
            state.source_size = sizeof group3_source;
            state.aux_mask = group3_mask;
            state.aux_mask_size = sizeof group3_mask;
            state.destination = destination;
            state.destination_size = sizeof destination;
            assert(pr_u40c_entry2_group3_tile_plane(&state, &report) ==
                   PR_U40C_GROUP3_TILE_OK);
            assert(report.item_index == call->index);
            assert(report.first_data_offset == first);
            assert(report.bytes_written == 32u);
            assert(report.bytes_changed == changed);
            assert(report.first_changed_offset == first + call->first_plane);
        } else {
            pr_u40c_tile_plane_state state = {0};
            pr_u40c_tile_plane_report report;
            state.item_id = call->item;
            state.group0_remap = 1u;
            state.x = call->x;
            state.y = call->y;
            state.flag = state.video_state = 1u;
            state.source = group0_source;
            state.source_size = sizeof group0_source;
            state.aux_mask = group0_mask;
            state.aux_mask_size = sizeof group0_mask;
            state.destination = destination;
            state.destination_size = sizeof destination;
            assert(pr_u40c_entry2_mode1_tile_plane(&state, &report) ==
                   PR_U40C_TILE_PLANE_OK);
            assert(report.item_index == call->index);
            assert(report.first_data_offset == first);
            assert(report.bytes_written == 32u);
            assert(report.bytes_changed == changed);
            assert(report.first_changed_offset ==
                   (changed ? first + call->first_plane : (size_t)-1));
        }

        for (row = 0; row < 8u; ++row)
            for (plane = call->first_plane; plane < 4u; ++plane)
                expected[HEADER + first + row * 21u * 4u + plane] = 0x32u;
        assert(memcmp(destination, expected, sizeof expected) == 0);
    }

    puts("saved view-four last-six persistent-object composition passed");
    return 0;
}
