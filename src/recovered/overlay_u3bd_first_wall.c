#include "poolrad/recovered/overlay_u3bd_first_wall.h"

#include "poolrad/recovered/overlay_u3bd_nibble.h"

#include <string.h>

#define U3BD_PIECE_SLICE 156u

pr_u3bd_first_wall_status pr_u3bd_entry5_first_wall(
    const pr_u3bd_first_wall_state *state, pr_u3bd_first_wall_draw draw,
    void *draw_ctx, pr_u3bd_first_wall_report *report)
{
    pr_u3bd_first_wall_report local;
    pr_u3bd_first_wall_report *r = report ? report : &local;
    pr_u3bd_nibble_state lookup;
    pr_u3bd_nibble_status lookup_status;
    int16_t first_x, first_y;
    int16_t grid_x, grid_y;
    uint8_t sample_x, sample_y;
    unsigned row, col;
    size_t first_item;
    uint8_t wall;

    if (!state) return PR_U3BD_FIRST_WALL_ARGUMENT;
    memset(r, 0, sizeof *r);
    if (!state->geo || state->geo_size < PR_U3BD_NIBBLE_BUFFER_SIZE)
        return PR_U3BD_FIRST_WALL_BOUNDS;

    /* +0x0656..+0x0682 and +0x0686..+0x06B2: signed party bytes, then
     * the first distance (2) times the facing-indexed signed step. */
    first_x = (int16_t)((int8_t)state->party_x +
                        2 * (int16_t)(int8_t)state->step_x);
    first_y = (int16_t)((int8_t)state->party_y +
                        2 * (int16_t)(int8_t)state->step_y);
    r->sample_x = first_x;
    r->sample_y = first_y;
    /* +0x06DB/+0x06EB push AL from each word local. The +0x031F leaf
     * compares those bytes as signed values; only 0..15 pass. */
    sample_x = (uint8_t)first_x;
    sample_y = (uint8_t)first_y;
    if (sample_x > 15u || sample_y > 15u)
        return PR_U3BD_FIRST_WALL_OTHER_PATH;

    /* +0x06C8..+0x06D8: the merged +0x047B leaf receives low bytes of
     * the word locals and the original facing byte. The first iteration's
     * previous-wall byte is zero, so +0x0709 skips its old-wall call.
     * For an in-grid sample, +0x06E9 also skips the second lookup. */
    lookup.map_id = state->map_id;
    lookup.buffer = state->geo;
    lookup.frame_residue = state->lookup_frame_residue;
    lookup_status = pr_u3bd_entry2(&lookup, sample_x,
                                   sample_y, state->facing,
                                   &wall, NULL);
    if (lookup_status == PR_U3BD_NIBBLE_ARGUMENT)
        return PR_U3BD_FIRST_WALL_BOUNDS;
    if (lookup_status == PR_U3BD_NIBBLE_UNSPECIFIED)
        return PR_U3BD_FIRST_WALL_LOOKUP_UNSPECIFIED;
    r->wall = wall;
    if (wall == 0) return PR_U3BD_FIRST_WALL_OK; /* +0x0707 -> +0x0744 */
    r->entered_draw = 1;

    /* +0x072C..+0x073F passes (DS:[0x0CAA], DS:[0x0CBE], wall, 0) to
     * local +0x0211. The helper reads the low byte of each position word. */
    grid_x = (int8_t)(uint8_t)state->view0_x;
    grid_y = (int8_t)state->view0_y;
    if (!state->view0_columns || !state->view0_rows ||
        (unsigned)state->view0_offset +
            (unsigned)state->view0_columns * state->view0_rows >
            U3BD_PIECE_SLICE)
        return PR_U3BD_FIRST_WALL_BOUNDS;
    first_item = (size_t)(wall - 1u) * U3BD_PIECE_SLICE +
                 state->view0_offset;

    /* +0x0257..+0x0318: row-major walk over the selected 156-byte slice.
     * The byte cursor advances even when clipping omits the piece read. */
    for (row = 0; row < state->view0_rows; ++row) {
        for (col = 0; col < state->view0_columns; ++col) {
            int16_t x = (int16_t)(grid_x + (int16_t)col);
            int16_t y = (int16_t)(grid_y + (int16_t)row);
            size_t at = first_item + row * state->view0_columns + col;
            uint8_t item;
            ++r->cells_seen;
            if (x < 0 || x > 10 || y < 0 || y > 10) continue;
            if (!state->pieces || at >= state->pieces_size)
                return PR_U3BD_FIRST_WALL_BOUNDS;
            item = state->pieces[at];
            if (!item) continue; /* +0x02BC: unsigned <= zero */
            if (!draw || draw(draw_ctx, (uint16_t)(x + 2),
                              (uint16_t)(y + 2), item, 1u, 1u))
                return PR_U3BD_FIRST_WALL_DRAW_HOOK;
            ++r->calls_done;
        }
    }
    return PR_U3BD_FIRST_WALL_OK;
}
