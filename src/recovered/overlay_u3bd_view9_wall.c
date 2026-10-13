#include "poolrad/recovered/overlay_u3bd_view9_wall.h"

#include "poolrad/recovered/overlay_u3bd_nibble.h"

#include <string.h>

#define U3BD_PIECE_SLICE 156u

static int16_t signed_byte(uint8_t value)
{
    return value < 128u ? (int16_t)value : (int16_t)((int)value - 256);
}

pr_u3bd_view9_wall_status pr_u3bd_entry5_view9_wall(
    const pr_u3bd_view9_wall_state *state, pr_u3bd_first_wall_draw draw,
    void *draw_ctx, pr_u3bd_view9_wall_report *report)
{
    pr_u3bd_view9_wall_report local;
    pr_u3bd_view9_wall_report *r = report ? report : &local;
    pr_u3bd_nibble_state lookup;
    pr_u3bd_nibble_status lookup_status;
    uint16_t origin_word;
    int16_t origin_x, origin_y;
    unsigned row, col;
    size_t first_item;

    if (!state) return PR_U3BD_VIEW9_WALL_ARGUMENT;
    memset(r, 0, sizeof *r);
    /* +0x0744 checks the old wall before reaching either data pointer. */
    if (!state->previous_wall) return PR_U3BD_VIEW9_WALL_OK;
    if (!state->geo || state->geo_size < PR_U3BD_NIBBLE_BUFFER_SIZE)
        return PR_U3BD_VIEW9_WALL_BOUNDS;

    /* +0x074A..+0x0777: sign-extend each facing step, subtract from
     * the two word locals, and pass only their low bytes to +0x047B. */
    r->sample_x = (uint16_t)(state->front_x - signed_byte(state->step_x));
    r->sample_y = (uint16_t)(state->front_y - signed_byte(state->step_y));
    lookup.map_id = state->map_id;
    lookup.buffer = state->geo;
    lookup.frame_residue = state->lookup_frame_residue;
    lookup_status = pr_u3bd_entry2(&lookup, (uint8_t)r->sample_x,
                                   (uint8_t)r->sample_y, state->facing,
                                   &r->sampled_wall, NULL);
    if (lookup_status == PR_U3BD_NIBBLE_ARGUMENT)
        return PR_U3BD_VIEW9_WALL_BOUNDS;
    if (lookup_status == PR_U3BD_NIBBLE_UNSPECIFIED)
        return PR_U3BD_VIEW9_WALL_LOOKUP_UNSPECIFIED;
    if (!r->sampled_wall) return PR_U3BD_VIEW9_WALL_OK;

    /* +0x077E..+0x0792 uses the *previous* wall, not the sampled
     * nonzero nibble, as the +0x0211 piece-slice selector. */
    r->entered_draw = 1;
    origin_word = (uint16_t)(state->view9_base_x +
                             state->lateral_offset + 1u);
    origin_x = signed_byte((uint8_t)origin_word);
    origin_y = signed_byte(state->view9_y);
    r->origin_x = origin_x;
    r->origin_y = origin_y;
    if (state->previous_wall > 15u ||
        (unsigned)state->view9_offset +
            (unsigned)state->view9_columns * state->view9_rows >
            U3BD_PIECE_SLICE)
        return PR_U3BD_VIEW9_WALL_BOUNDS;
    first_item = (size_t)(state->previous_wall - 1u) *
                 U3BD_PIECE_SLICE + state->view9_offset;

    /* Local +0x0211 walks y outside x. The byte cursor advances even
     * when clipping omits the piece read or the item is zero. */
    for (row = 0; row < state->view9_rows; ++row) {
        for (col = 0; col < state->view9_columns; ++col) {
            int x = origin_x + (int)col;
            int y = origin_y + (int)row;
            size_t at = first_item + row * state->view9_columns + col;
            uint8_t item;
            ++r->cells_seen;
            if (x < 0 || x > 10 || y < 0 || y > 10) continue;
            if (!state->pieces || at >= state->pieces_size)
                return PR_U3BD_VIEW9_WALL_BOUNDS;
            item = state->pieces[at];
            if (!item) continue;
            if (!draw || draw(draw_ctx, (uint16_t)(x + 2),
                              (uint16_t)(y + 2), item, 1u, 1u))
                return PR_U3BD_VIEW9_WALL_DRAW_HOOK;
            ++r->calls_done;
        }
    }
    return PR_U3BD_VIEW9_WALL_OK;
}
