#include "poolrad/recovered/overlay_u3bd_view1_scan.h"

#include "poolrad/recovered/overlay_u3bd_nibble.h"

#include <string.h>

#define U3BD_PIECE_SLICE 156u

static int signed_byte(uint8_t b)
{
    return b < 128u ? (int)b : (int)b - 256;
}

static pr_u3bd_view1_scan_status place_view1(
    const pr_u3bd_view1_scan_state *s, uint8_t wall, uint16_t origin_word,
    pr_u3bd_first_wall_draw draw, void *draw_ctx,
    pr_u3bd_view1_scan_report *r)
{
    int origin_x = signed_byte((uint8_t)origin_word);
    int origin_y = signed_byte(s->view1_y);
    size_t first_item;
    unsigned row, col;

    ++r->placements;
    if (wall > 15u ||
        (unsigned)s->view1_offset +
            (unsigned)s->view1_columns * s->view1_rows > U3BD_PIECE_SLICE)
        return PR_U3BD_VIEW1_SCAN_BOUNDS;
    first_item = (size_t)(wall - 1u) * U3BD_PIECE_SLICE + s->view1_offset;
    /* Local +0x0211 walks rows outside columns. Clipping precedes the
     * piece read, while its cursor advances through every cell. */
    for (row = 0; row < s->view1_rows; ++row) {
        for (col = 0; col < s->view1_columns; ++col) {
            int x = origin_x + (int)col;
            int y = origin_y + (int)row;
            size_t at = first_item + row * s->view1_columns + col;
            uint8_t item;
            ++r->cells_seen;
            if (x < 0 || x > 10 || y < 0 || y > 10) continue;
            if (!s->pieces || at >= s->pieces_size)
                return PR_U3BD_VIEW1_SCAN_BOUNDS;
            item = s->pieces[at];
            if (!item) continue;
            if (!draw || draw(draw_ctx, (uint16_t)(x + 2),
                              (uint16_t)(y + 2), item, 1u, 1u))
                return PR_U3BD_VIEW1_SCAN_DRAW_HOOK;
            ++r->calls_done;
        }
    }
    return PR_U3BD_VIEW1_SCAN_OK;
}

pr_u3bd_view1_scan_status pr_u3bd_entry5_view1_scan(
    const pr_u3bd_view1_scan_state *s, pr_u3bd_first_wall_draw draw,
    void *draw_ctx, pr_u3bd_view1_scan_report *report)
{
    pr_u3bd_view1_scan_report local;
    pr_u3bd_view1_scan_report *r = report ? report : &local;
    pr_u3bd_nibble_state lookup;
    uint16_t x, y, lateral = 0;
    unsigned i;

    if (!s) return PR_U3BD_VIEW1_SCAN_ARGUMENT;
    memset(r, 0, sizeof *r);
    x = s->start_x;
    y = s->start_y;
    lookup.map_id = s->map_id;
    lookup.buffer = s->geo;
    lookup.frame_residue = s->lookup_frame_residue;
    /* +0x08E8 copies the start words; +0x08F4 clears the loop byte;
     * +0x08F8 clears the lateral word. The loop body begins at +0x08FD. */
    for (i = 0; i < 3u; ++i) {
        pr_u3bd_nibble_status ns;
        pr_u3bd_view1_scan_status ps;
        uint8_t wall;
        r->sample_x[i] = x;
        r->sample_y[i] = y;
        /* The native call reads a byte from each word local. */
        if (!s->geo || s->geo_size < PR_U3BD_NIBBLE_BUFFER_SIZE)
            return PR_U3BD_VIEW1_SCAN_BOUNDS;
        ns = pr_u3bd_entry2(&lookup, (uint8_t)x, (uint8_t)y,
                            s->facing, &wall, NULL);
        if (ns == PR_U3BD_NIBBLE_ARGUMENT)
            return PR_U3BD_VIEW1_SCAN_BOUNDS;
        if (ns == PR_U3BD_NIBBLE_UNSPECIFIED)
            return PR_U3BD_VIEW1_SCAN_LOOKUP_UNSPECIFIED;
        r->wall[i] = wall;
        ++r->samples_done;
        /* +0x0916 skips placement for zero. At +0x091C the first
         * iteration uses base+lateral; later ones subtract one. */
        if (wall) {
            uint16_t origin = (uint16_t)(s->view1_base_x + lateral);
            if (i) origin = (uint16_t)(origin - 1u);
            ps = place_view1(s, wall, origin, draw, draw_ctx, r);
            if (ps != PR_U3BD_VIEW1_SCAN_OK) return ps;
        }
        /* +0x0951..+0x097C advances even after a zero wall. */
        lateral = (uint16_t)(lateral - 2u);
        x = (uint16_t)(x + signed_byte(s->step_x));
        y = (uint16_t)(y + signed_byte(s->step_y));
    }
    r->final_x = x;
    r->final_y = y;
    r->final_lateral = lateral;
    return PR_U3BD_VIEW1_SCAN_OK;
}
