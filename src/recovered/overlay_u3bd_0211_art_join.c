#include "poolrad/recovered/overlay_u3bd_0211_art_join.h"

#include <string.h>

#define U3BD_PIECE_SLICE 156u

pr_u3bd_0211_art_join_status pr_u3bd_0211_art_join(
    const pr_u3bd_0211_art_join_state *state,
    pr_u3bd_0211_art_join_report *report)
{
    pr_u3bd_0211_art_join_report local;
    pr_u3bd_0211_art_join_report *r = report ? report : &local;
    size_t first_item;
    unsigned row, col;
    int16_t origin_x, origin_y;

    if (!state) return PR_U3BD_0211_ART_ARGUMENT;
    memset(r, 0, sizeof *r);
    if (!state->wall) return PR_U3BD_0211_ART_OTHER_PATH;
    if (!state->view0_columns || !state->view0_rows ||
        (unsigned)state->view0_offset +
            (unsigned)state->view0_columns * state->view0_rows >
                U3BD_PIECE_SLICE)
        return PR_U3BD_0211_ART_BOUNDS;

    /* +0x0211 uses the low signed byte of the origin words. The selected
     * 156-byte piece slice is one-based, while the table offset is zero-based. */
    origin_x = (int8_t)(uint8_t)state->origin_x;
    origin_y = (int8_t)state->origin_y;
    first_item = (size_t)(state->wall - 1u) * U3BD_PIECE_SLICE +
                 state->view0_offset;

    for (row = 0; row < state->view0_rows; ++row) {
        for (col = 0; col < state->view0_columns; ++col) {
            int16_t x = (int16_t)(origin_x + (int16_t)col);
            int16_t y = (int16_t)(origin_y + (int16_t)row);
            size_t at = first_item + row * state->view0_columns + col;
            uint8_t item;
            pr_u40c_tile_plane_state tile;
            pr_u40c_tile_plane_report tile_report;
            pr_u40c_tile_plane_status tile_status;

            ++r->cells_seen;
            if (x < 0 || x > 10 || y < 0 || y > 10) continue;
            if (!state->pieces || at >= state->pieces_size)
                return PR_U3BD_0211_ART_BOUNDS;
            item = state->pieces[at];
            if (!item) continue;

            /* +0x02C4..+0x02FB shifts each grid coordinate by two and
             * passes (x,y,item,1,1) to overlay 0x428:0x002A. */
            tile = state->tile;
            tile.item_id = item;
            tile.x = (uint8_t)(x + 2);
            tile.y = (uint8_t)(y + 2);
            tile_status = pr_u40c_entry2_mode1_tile_plane(&tile, &tile_report);
            if (tile_status == PR_U40C_TILE_PLANE_BOUNDS ||
                tile_status == PR_U40C_TILE_PLANE_ARGUMENT)
                return PR_U3BD_0211_ART_BOUNDS;
            if (tile_status != PR_U40C_TILE_PLANE_OK)
                return PR_U3BD_0211_ART_OTHER_PATH;

            if (!r->tile_calls) {
                r->first_item = item;
                r->first_x = tile.x;
                r->first_y = tile.y;
            }
            r->last_item = item;
            r->last_x = tile.x;
            r->last_y = tile.y;
            ++r->tile_calls;
            r->plane_bytes_written += tile_report.bytes_written;
            r->plane_bytes_changed += tile_report.bytes_changed;
        }
    }
    return PR_U3BD_0211_ART_OK;
}
