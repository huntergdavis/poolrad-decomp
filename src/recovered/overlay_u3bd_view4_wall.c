#include "poolrad/recovered/overlay_u3bd_view4_wall.h"

#include "poolrad/recovered/overlay_u3bd_nibble.h"

#include <string.h>

#define U3BD_PIECE_SLICE 156u

static int16_t signed_byte(uint8_t value)
{
    return value < 128u ? (int16_t)value : (int16_t)((int)value - 256);
}

pr_u3bd_view4_wall_status pr_u3bd_entry5_view4_wall(
    const pr_u3bd_view4_wall_state *state, pr_u3bd_first_wall_draw draw,
    void *draw_ctx, pr_u3bd_view4_wall_report *report)
{
    pr_u3bd_view4_wall_report local;
    pr_u3bd_view4_wall_report *r = report ? report : &local;
    pr_u3bd_nibble_state lookup;
    pr_u3bd_nibble_status lookup_status;
    uint16_t origin_word;
    size_t first_item;
    unsigned row, col;

    if (!state) return PR_U3BD_VIEW4_WALL_ARGUMENT;
    memset(r, 0, sizeof *r);
    if (!state->geo || state->geo_size < PR_U3BD_NIBBLE_BUFFER_SIZE)
        return PR_U3BD_VIEW4_WALL_BOUNDS;

    /* +0x0AA9..+0x0AC0: the frame words are narrowed to AL, and the
     * facing byte [BP-1] selects the quarter/nibble at entry 2. */
    lookup.map_id = state->map_id;
    lookup.buffer = state->geo;
    lookup.frame_residue = state->lookup_frame_residue;
    lookup_status = pr_u3bd_entry2(&lookup, (uint8_t)state->sample_x,
                                   (uint8_t)state->sample_y, state->facing,
                                   &r->wall, NULL);
    if (lookup_status == PR_U3BD_NIBBLE_ARGUMENT)
        return PR_U3BD_VIEW4_WALL_BOUNDS;
    if (lookup_status == PR_U3BD_NIBBLE_UNSPECIFIED)
        return PR_U3BD_VIEW4_WALL_LOOKUP_UNSPECIFIED;
    if (!r->wall) return PR_U3BD_VIEW4_WALL_OK;
    r->entered_draw = 1;

    /* +0x0AC2..+0x0AD5 passes base+lateral, y, wall, and view 4 to
     * +0x0211. That helper interprets only the origin's signed low bytes. */
    origin_word = (uint16_t)(state->view4_base_x + state->lateral_offset);
    r->origin_x = signed_byte((uint8_t)origin_word);
    r->origin_y = signed_byte(state->view4_y);
    if (r->wall > 15u || !state->view4_columns || !state->view4_rows ||
        (unsigned)state->view4_offset +
            (unsigned)state->view4_columns * state->view4_rows >
            U3BD_PIECE_SLICE)
        return PR_U3BD_VIEW4_WALL_BOUNDS;
    first_item = (size_t)(r->wall - 1u) * U3BD_PIECE_SLICE +
                 state->view4_offset;

    /* +0x0257..+0x0318: row-major cursor, including clipped/zero cells.
     * A positive visible item reaches 0x428:002A with two trailing ones. */
    for (row = 0; row < state->view4_rows; ++row) {
        for (col = 0; col < state->view4_columns; ++col) {
            int x = r->origin_x + (int)col;
            int y = r->origin_y + (int)row;
            size_t at = first_item + row * state->view4_columns + col;
            uint8_t item;
            ++r->cells_seen;
            if (x < 0 || x > 10 || y < 0 || y > 10) continue;
            if (!state->pieces || at >= state->pieces_size)
                return PR_U3BD_VIEW4_WALL_BOUNDS;
            item = state->pieces[at];
            if (!item) continue;
            if (!draw || draw(draw_ctx, (uint16_t)(x + 2),
                              (uint16_t)(y + 2), item, 1u, 1u))
                return PR_U3BD_VIEW4_WALL_DRAW_HOOK;
            ++r->calls_done;
        }
    }
    return PR_U3BD_VIEW4_WALL_OK;
}
