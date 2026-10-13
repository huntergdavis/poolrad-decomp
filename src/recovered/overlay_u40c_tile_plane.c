#include "poolrad/recovered/overlay_u40c_tile_plane.h"

#include <string.h>

#define U40C_HEADER_SIZE 0x17u
#define U40C_TILE_ROWS 8u
#define U40C_PLANES 4u
#define U40C_ITEM_BYTES 32u
#define U40C_MASK_BYTES 8u
#define U40C_DEST_WIDTH 21u
#define U40C_DEST_HEIGHT 168u

static uint16_t word(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8));
}

pr_u40c_tile_plane_status pr_u40c_entry2_mode1_tile_plane(
    const pr_u40c_tile_plane_state *state, pr_u40c_tile_plane_report *report)
{
    pr_u40c_tile_plane_report local;
    pr_u40c_tile_plane_report *r = report ? report : &local;
    uint16_t index;
    unsigned row, plane;

    if (!state) return PR_U40C_TILE_PLANE_ARGUMENT;
    memset(r, 0, sizeof *r);
    r->first_changed_offset = (size_t)-1;

    /* Entry 2 +0x0163..+0x021A: saved IDs 1 and 13 select group zero,
     * pass the nonnull group pointer, subtract the remap word, and take
     * the [BP+8] != 0 branch. The reached flag is one. */
    if (state->item_id < 1u || state->item_id > 45u ||
        state->flag != 1u || state->video_state != 1u ||
        state->x >= U40C_DEST_WIDTH ||
        (unsigned)state->y * U40C_TILE_ROWS + U40C_TILE_ROWS >
            U40C_DEST_HEIGHT)
        return PR_U40C_TILE_PLANE_OTHER_PATH;
    if (state->item_id < state->group0_remap)
        return PR_U40C_TILE_PLANE_OTHER_PATH;
    index = (uint16_t)(state->item_id - state->group0_remap);
    if (index > 255u) return PR_U40C_TILE_PLANE_OTHER_PATH;
    r->item_index = index;

    /* RES_0464:08D4 +0x08DA..+0x09DD: auxiliary far pointer exists;
     * source height 8, width 1, item stride 32; destination width 21.
     * The private replay also measured destination height 168. */
    if (!state->source || !state->destination ||
        state->source_size < U40C_HEADER_SIZE ||
        state->destination_size < U40C_HEADER_SIZE)
        return PR_U40C_TILE_PLANE_BOUNDS;
    if (word(state->source + 0x13u) == 0u &&
        word(state->source + 0x15u) == 0u)
        return PR_U40C_TILE_PLANE_OTHER_PATH;
    if (word(state->source) != U40C_TILE_ROWS ||
        word(state->source + 2u) != 1u ||
        word(state->source + 0x11u) != U40C_ITEM_BYTES ||
        word(state->destination) != U40C_DEST_HEIGHT ||
        word(state->destination + 2u) != U40C_DEST_WIDTH)
        return PR_U40C_TILE_PLANE_OTHER_PATH;
    if (!state->aux_mask ||
        state->source_size < U40C_HEADER_SIZE +
            ((size_t)index + 1u) * U40C_ITEM_BYTES ||
        state->aux_mask_size < ((size_t)index + 1u) * U40C_MASK_BYTES ||
        state->destination_size < U40C_HEADER_SIZE +
            U40C_DEST_WIDTH * U40C_DEST_HEIGHT * U40C_PLANES)
        return PR_U40C_TILE_PLANE_BOUNDS;

    /* +0x08F9 and +0x0B7B..+0x0BC7: y is scaled by eight before the
     * 21-column destination stride; x is scaled by four plane bytes. */
    r->first_data_offset =
        ((size_t)state->y * U40C_TILE_ROWS * U40C_DEST_WIDTH +
         state->x) * U40C_PLANES;

    /* +0x0BEB..+0x0CB9, mode 1 with auxiliary data: the byte at +0x0C6A
     * receives (old destination AND mask[row]) OR source[item,row,plane].
     * This covers only the destination object data, not the DS metadata
     * writes interleaved with the original outer loop. */
    for (row = 0; row < U40C_TILE_ROWS; ++row) {
        for (plane = 0; plane < U40C_PLANES; ++plane) {
            size_t data_offset = r->first_data_offset +
                                 row * U40C_DEST_WIDTH * U40C_PLANES + plane;
            uint8_t *destination = state->destination + U40C_HEADER_SIZE +
                                   data_offset;
            uint8_t before = *destination;
            uint8_t mask = state->aux_mask[(size_t)index *
                                           U40C_MASK_BYTES + row];
            uint8_t source = state->source[U40C_HEADER_SIZE +
                                           (size_t)index * U40C_ITEM_BYTES +
                                           row * U40C_PLANES + plane];
            uint8_t after = (uint8_t)((before & mask) | source);
            *destination = after;
            ++r->bytes_written;
            if (after != before) {
                ++r->bytes_changed;
                if (r->first_changed_offset == (size_t)-1)
                    r->first_changed_offset = data_offset;
            }
        }
    }
    return PR_U40C_TILE_PLANE_OK;
}
