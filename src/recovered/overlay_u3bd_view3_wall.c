#include "poolrad/recovered/overlay_u3bd_view3_wall.h"

#include "poolrad/recovered/overlay_u3bd_nibble.h"

#include <string.h>

#define U3BD_PIECE_SLICE 156u

static int16_t signed_byte(uint8_t value)
{
    return value < 128u ? (int16_t)value : (int16_t)((int)value - 256);
}

pr_u3bd_view3_wall_status pr_u3bd_entry5_view3_wall(
    const pr_u3bd_view3_wall_state *state, pr_u3bd_first_wall_draw draw,
    void *draw_ctx, pr_u3bd_view3_wall_report *report)
{
    pr_u3bd_view3_wall_report local;
    pr_u3bd_view3_wall_report *r = report ? report : &local;
    pr_u3bd_nibble_state lookup;
    pr_u3bd_nibble_status lookup_status;
    uint16_t origin_word;
    size_t first_item;
    unsigned row, col;

    if (!state) return PR_U3BD_VIEW3_WALL_ARGUMENT;
    memset(r, 0, sizeof *r);
    if (!state->geo || state->geo_size < PR_U3BD_NIBBLE_BUFFER_SIZE)
        return PR_U3BD_VIEW3_WALL_BOUNDS;

    /* +0x0A7A..+0x0A91: the frame's sample word low bytes and [BP+6]
     * feed entry 2. Zero skips placement entirely. */
    lookup.map_id = state->map_id;
    lookup.buffer = state->geo;
    lookup.frame_residue = state->lookup_frame_residue;
    lookup_status = pr_u3bd_entry2(&lookup, (uint8_t)state->sample_x,
                                   (uint8_t)state->sample_y,
                                   state->selector, &r->wall, NULL);
    if (lookup_status == PR_U3BD_NIBBLE_ARGUMENT)
        return PR_U3BD_VIEW3_WALL_BOUNDS;
    if (lookup_status == PR_U3BD_NIBBLE_UNSPECIFIED)
        return PR_U3BD_VIEW3_WALL_LOOKUP_UNSPECIFIED;
    if (!r->wall) return PR_U3BD_VIEW3_WALL_OK;
    r->entered_draw = 1;

    /* +0x0A93..+0x0AA6 pushes base+lateral, y, sampled wall, view 3.
     * +0x0211 interprets the origin's low bytes as signed grid cells. */
    origin_word = (uint16_t)(state->view3_base_x + state->lateral_offset);
    r->origin_x = signed_byte((uint8_t)origin_word);
    r->origin_y = signed_byte(state->view3_y);
    if (r->wall > 15u || !state->view3_columns || !state->view3_rows ||
        (unsigned)state->view3_offset +
            (unsigned)state->view3_columns * state->view3_rows >
            U3BD_PIECE_SLICE)
        return PR_U3BD_VIEW3_WALL_BOUNDS;
    first_item = (size_t)(r->wall - 1u) * U3BD_PIECE_SLICE +
                 state->view3_offset;

    /* +0x0257..+0x0318 advances the piece cursor across clipped cells.
     * Only positive, visible items reach 0x428:002A. */
    for (row = 0; row < state->view3_rows; ++row) {
        for (col = 0; col < state->view3_columns; ++col) {
            int x = r->origin_x + (int)col;
            int y = r->origin_y + (int)row;
            size_t at = first_item + row * state->view3_columns + col;
            uint8_t item;
            ++r->cells_seen;
            if (x < 0 || x > 10 || y < 0 || y > 10) continue;
            if (!state->pieces || at >= state->pieces_size)
                return PR_U3BD_VIEW3_WALL_BOUNDS;
            item = state->pieces[at];
            if (!item) continue;
            if (!draw || draw(draw_ctx, (uint16_t)(x + 2),
                              (uint16_t)(y + 2), item, 1u, 1u))
                return PR_U3BD_VIEW3_WALL_DRAW_HOOK;
            ++r->calls_done;
        }
    }
    return PR_U3BD_VIEW3_WALL_OK;
}
