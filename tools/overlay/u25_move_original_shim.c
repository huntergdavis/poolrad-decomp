/* Plain-argument shim over pr_u25_entry19 (src/recovered/overlay_u25_move.c) for tools/overlay/u25_move_original_audit.py.
 * io[5] in/out = x [6AAD], y [6AAE], facing [6AAF], cell [6AB1], nibble [6AB0]. */
#include "poolrad/recovered/overlay_u25_move.h"

int shim_u25_entry19(uint8_t io[5], uint8_t map_id, const uint8_t *map_buffer, uint8_t residue)
{
    pr_u25_move_state s;
    int status;
    s.x = io[0]; s.y = io[1]; s.facing = io[2]; s.cell = io[3]; s.nibble = io[4];
    s.map_id = map_id; s.map_buffer = map_buffer; s.nibble_frame_residue = residue;
    status = (int)pr_u25_entry19(&s);
    io[0] = s.x; io[1] = s.y; io[2] = s.facing; io[3] = s.cell; io[4] = s.nibble;
    return status;
}
