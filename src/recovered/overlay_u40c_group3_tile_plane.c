#include "poolrad/recovered/overlay_u40c_group3_tile_plane.h"

#include <string.h>

#define HEADER_SIZE 0x17u
#define TILE_ROWS 8u
#define PLANES 4u
#define ITEM_BYTES 32u
#define MASK_BYTES 8u
#define DEST_WIDTH 21u
#define DEST_HEIGHT 168u

static uint16_t word(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8));
}

pr_u40c_group3_tile_plane_status pr_u40c_entry2_group3_tile_plane(
    const pr_u40c_group3_tile_plane_state *state,
    pr_u40c_group3_tile_plane_report *report)
{
    pr_u40c_group3_tile_plane_report local;
    pr_u40c_group3_tile_plane_report *r = report ? report : &local;
    uint16_t index;
    unsigned row, plane;

    if (!state) return PR_U40C_GROUP3_TILE_ARGUMENT;
    memset(r, 0, sizeof *r);
    r->first_changed_offset = (size_t)-1;

    /* Entry 2 +0x0196..+0x01A4 selects group three. +0x020F reads its
     * remap word, and +0x0216 takes the nonzero flag's far-call branch. */
    if (state->item_id < 0xBAu || state->item_id > 0xFFu ||
        state->flag != 1u || state->video_state != 1u ||
        state->x >= DEST_WIDTH ||
        (unsigned)state->y * TILE_ROWS + TILE_ROWS > DEST_HEIGHT ||
        state->item_id < state->group3_remap)
        return PR_U40C_GROUP3_TILE_OTHER_PATH;
    index = (uint16_t)(state->item_id - state->group3_remap);
    if (index > 255u) return PR_U40C_GROUP3_TILE_OTHER_PATH;
    r->item_index = index;

    if (!state->source || !state->destination ||
        state->source_size < HEADER_SIZE ||
        state->destination_size < HEADER_SIZE)
        return PR_U40C_GROUP3_TILE_BOUNDS;

    /* RES_0464:08D4 +0x08E3 ORs both words of the auxiliary far pointer.
     * Its offset may be zero while its segment is nonzero. */
    if ((word(state->source + 0x13u) |
         word(state->source + 0x15u)) == 0u ||
        word(state->source) != TILE_ROWS ||
        word(state->source + 2u) != 1u ||
        word(state->source + 0x11u) != ITEM_BYTES ||
        word(state->destination) != DEST_HEIGHT ||
        word(state->destination + 2u) != DEST_WIDTH)
        return PR_U40C_GROUP3_TILE_OTHER_PATH;
    if (!state->aux_mask ||
        state->source_size < HEADER_SIZE +
            ((size_t)index + 1u) * ITEM_BYTES ||
        state->aux_mask_size < ((size_t)index + 1u) * MASK_BYTES ||
        state->destination_size < HEADER_SIZE +
            DEST_WIDTH * DEST_HEIGHT * PLANES)
        return PR_U40C_GROUP3_TILE_BOUNDS;

    /* +0x08F9 and +0x0B7B scale y by eight and x by four plane bytes. */
    r->first_data_offset =
        ((size_t)state->y * TILE_ROWS * DEST_WIDTH + state->x) * PLANES;

    /* RES_0464:08D4 +0x0BCA..+0x0CB9, auxiliary-present mode 1.
     * The byte at +0x0C6A receives (old AND row mask) OR source byte. */
    for (row = 0; row < TILE_ROWS; ++row) {
        for (plane = 0; plane < PLANES; ++plane) {
            size_t offset = r->first_data_offset +
                            row * DEST_WIDTH * PLANES + plane;
            uint8_t *dst = state->destination + HEADER_SIZE + offset;
            uint8_t before = *dst;
            uint8_t mask = state->aux_mask[(size_t)index * MASK_BYTES + row];
            uint8_t src = state->source[HEADER_SIZE +
                                        (size_t)index * ITEM_BYTES +
                                        row * PLANES + plane];
            uint8_t after = (uint8_t)((before & mask) | src);
            *dst = after;
            ++r->bytes_written;
            if (after != before) {
                ++r->bytes_changed;
                if (r->first_changed_offset == (size_t)-1)
                    r->first_changed_offset = offset;
            }
        }
    }
    return PR_U40C_GROUP3_TILE_OK;
}
