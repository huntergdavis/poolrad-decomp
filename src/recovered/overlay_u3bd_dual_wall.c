#include "poolrad/recovered/overlay_u3bd_dual_wall.h"

#include <string.h>

#define U3BD_PIECE_SLICE 156u

static int signed_low_byte(uint16_t word)
{
    unsigned byte = (uint8_t)word;
    return byte < 128u ? (int)byte : (int)byte - 256;
}

/* Exact reached +0x0211 row-major cursor and 0..10 signed clipping. */
static pr_u3bd_dual_wall_status place(
    const pr_u3bd_dual_wall_state *state,
    const pr_u3bd_dual_wall_view *view,
    uint16_t origin_word, uint8_t wall,
    pr_u3bd_first_wall_draw draw, void *draw_ctx,
    pr_u3bd_dual_wall_report *report)
{
    int origin_x = signed_low_byte(origin_word);
    int origin_y = signed_low_byte(view->y);
    size_t first;
    unsigned row, col;

    if (wall < 1u || wall > 15u ||
        (unsigned)view->offset + (unsigned)view->columns * view->rows >
            U3BD_PIECE_SLICE)
        return PR_U3BD_DUAL_WALL_BOUNDS;
    first = (size_t)(wall - 1u) * U3BD_PIECE_SLICE + view->offset;
    for (row = 0; row < view->rows; ++row) {
        for (col = 0; col < view->columns; ++col) {
            int x = origin_x + (int)col;
            int y = origin_y + (int)row;
            size_t at = first + row * view->columns + col;
            uint8_t item;
            ++report->cells_seen;
            if (x < 0 || x > 10 || y < 0 || y > 10) continue;
            if (!state->pieces || at >= state->pieces_size)
                return PR_U3BD_DUAL_WALL_BOUNDS;
            item = state->pieces[at];
            if (!item) continue;
            if (!draw || draw(draw_ctx, (uint16_t)(x + 2),
                              (uint16_t)(y + 2), item, 1u, 1u))
                return PR_U3BD_DUAL_WALL_DRAW_HOOK;
            ++report->calls_done;
        }
    }
    return PR_U3BD_DUAL_WALL_OK;
}

pr_u3bd_dual_wall_status pr_u3bd_entry5_dual_wall(
    const pr_u3bd_dual_wall_state *state, pr_u3bd_first_wall_draw draw,
    void *draw_ctx, pr_u3bd_dual_wall_report *report)
{
    pr_u3bd_dual_wall_report local;
    pr_u3bd_dual_wall_report *r = report ? report : &local;
    pr_u3bd_dual_wall_status status;
    uint16_t origin;

    if (!state) return PR_U3BD_DUAL_WALL_ARGUMENT;
    memset(r, 0, sizeof *r);
    r->previous_wall_after = state->previous_wall;
    if (!state->current_wall) return PR_U3BD_DUAL_WALL_OTHER_PATH;

    /* +0x082A..+0x0847: previous wall, view 9, lateral -1. */
    if (state->previous_wall) {
        r->view9_entered = 1;
        origin = (uint16_t)(state->view9.base_x +
                            state->lateral_offset - 1u);
        status = place(state, &state->view9, origin, state->previous_wall,
                       draw, draw_ctx, r);
        if (status != PR_U3BD_DUAL_WALL_OK) return status;
    }

    /* +0x0847..+0x0863: commit current wall before view-zero call. */
    r->previous_wall_after = state->current_wall;
    r->view0_entered = 1;
    origin = (uint16_t)(state->view0.base_x + state->lateral_offset);
    return place(state, &state->view0, origin, state->current_wall,
                 draw, draw_ctx, r);
}
