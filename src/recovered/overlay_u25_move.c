#include "poolrad/recovered/overlay_u25_move.h"
#include "poolrad/recovered/overlay_u3bd_cell.h"
#include "poolrad/recovered/overlay_u3bd_nibble.h"

static int signed_byte(uint8_t b)
{
    return b < 0x80u ? (int)b : (int)b - 256;
}

pr_u25_move_status pr_u25_entry19(pr_u25_move_state *state)
{
    pr_u3bd_cell_state cell_state;
    pr_u3bd_nibble_state nibble_state;
    pr_u3bd_nibble_status nibble_status;
    uint8_t value;
    if (!state) return PR_U25_MOVE_ARGUMENT;

    /* +0x19DE..+0x1A3E: compare facing in order 0, 2, 4, 6; use signed
     * byte comparisons on x/y, then write one byte or leave both unchanged. */
    switch (state->facing) {
    case 0:
        state->y = signed_byte(state->y) > 0 ? (uint8_t)(state->y - 1u) : 15u;
        break;
    case 2:
        state->x = signed_byte(state->x) < 15 ? (uint8_t)(state->x + 1u) : 0u;
        break;
    case 4:
        state->y = signed_byte(state->y) < 15 ? (uint8_t)(state->y + 1u) : 0u;
        break;
    case 6:
        state->x = signed_byte(state->x) > 0 ? (uint8_t)(state->x - 1u) : 15u;
        break;
    default:
        break;
    }

    /* +0x1A3F..+0x1A4F: push x, y; call unit 0x3BD entry 4; store AL. */
    cell_state.map_id = state->map_id;
    cell_state.buffer = state->map_buffer;
    if (pr_u3bd_entry4(&cell_state, state->x, state->y, &value, 0) != PR_U3BD_CELL_OK)
        return PR_U25_MOVE_ARGUMENT;
    state->cell = value;

    /* +0x1A4F..+0x1A63: push x, y, facing; call entry 2; store AL even
     * when that callee returns its unwritten stack-slot residue. */
    nibble_state.map_id = state->map_id;
    nibble_state.buffer = state->map_buffer;
    nibble_state.frame_residue = state->nibble_frame_residue;
    nibble_status = pr_u3bd_entry2(&nibble_state, state->x, state->y,
                                   state->facing, &value, 0);
    if (nibble_status == PR_U3BD_NIBBLE_ARGUMENT) return PR_U25_MOVE_ARGUMENT;
    state->nibble = value;
    return nibble_status == PR_U3BD_NIBBLE_UNSPECIFIED
           ? PR_U25_MOVE_UNSPECIFIED : PR_U25_MOVE_OK;
}
