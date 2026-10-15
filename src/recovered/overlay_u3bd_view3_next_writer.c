#include "poolrad/recovered/overlay_u3bd_view3_next_writer.h"

#include "poolrad/recovered/overlay_u3bd_nibble.h"
#include "poolrad/recovered/overlay_u3bd_view3_wall.h"

#include <string.h>

static pr_u3bd_view3_next_writer_status nibble(
    const pr_u3bd_view3_next_writer_state *state, uint16_t x, uint16_t y,
    uint8_t selector, uint8_t *wall)
{
    pr_u3bd_nibble_state lookup;
    pr_u3bd_nibble_status result;

    lookup.map_id = state->map_id;
    lookup.buffer = state->geo;
    lookup.frame_residue = state->lookup_frame_residue;
    result = pr_u3bd_entry2(&lookup, (uint8_t)x, (uint8_t)y, selector,
                            wall, NULL);
    if (result == PR_U3BD_NIBBLE_UNSPECIFIED)
        return PR_U3BD_VIEW3_NEXT_LOOKUP_UNSPECIFIED;
    if (result != PR_U3BD_NIBBLE_OK)
        return PR_U3BD_VIEW3_NEXT_BOUNDS;
    return PR_U3BD_VIEW3_NEXT_OK;
}

static int16_t signed_byte(uint8_t value)
{
    return value < 128u ? (int16_t)value : (int16_t)((int)value - 256);
}

pr_u3bd_view3_next_writer_status pr_u3bd_entry5_view3_next_writer(
    const pr_u3bd_view3_next_writer_state *state,
    pr_u3bd_first_wall_draw draw, void *draw_ctx,
    pr_u3bd_view3_next_writer_report *report)
{
    pr_u3bd_view3_next_writer_report local;
    pr_u3bd_view3_next_writer_report *r = report ? report : &local;
    pr_u3bd_view3_next_writer_status status;
    uint8_t wall;
    unsigned iteration;

    memset(r, 0, sizeof *r);
    if (!state) return PR_U3BD_VIEW3_NEXT_ARGUMENT;
    r->sample_x = state->sample_x;
    r->sample_y = state->sample_y;
    r->lateral_offset = state->lateral_offset;
    r->scan_index = state->scan_index;
    if (!state->geo || state->geo_size < PR_U3BD_NIBBLE_BUFFER_SIZE ||
        !state->pieces || !state->x_shift || !state->y_shift ||
        state->direction_index >= state->shift_count)
        return PR_U3BD_VIEW3_NEXT_BOUNDS;
    if (state->scan_index != 0u) return PR_U3BD_VIEW3_NEXT_OTHER_PATH;

    /* +0x0AA9..+0x0AD8: the reached first view-four lookup was zero.
     * A nonzero selector has a different placement before this prefix. */
    status = nibble(state, r->sample_x, r->sample_y, state->facing, &wall);
    if (status != PR_U3BD_VIEW3_NEXT_OK) return status;
    if (wall != 0u) return PR_U3BD_VIEW3_NEXT_OTHER_PATH;

    for (iteration = 0; iteration < 2u; ++iteration) {
        pr_u3bd_view3_wall_state view;
        pr_u3bd_view3_wall_report view_report;
        pr_u3bd_view3_wall_status view_status;
        int16_t dx = signed_byte(state->x_shift[state->direction_index]);
        int16_t dy = signed_byte(state->y_shift[state->direction_index]);

        /* +0x0AD8..+0x0B03: increment the scan index, advance the
         * lateral offset by three, and signed-add the two table bytes. */
        r->scan_index = (uint8_t)(r->scan_index + 1u);
        r->lateral_offset = (uint16_t)(r->lateral_offset + 3u);
        r->sample_x = (uint16_t)(r->sample_x + (uint16_t)dx);
        r->sample_y = (uint16_t)(r->sample_y + (uint16_t)dy);

        memset(&view, 0, sizeof view);
        view.sample_x = r->sample_x;
        view.sample_y = r->sample_y;
        view.selector = state->outer_selector;
        view.map_id = state->map_id;
        view.geo = state->geo;
        view.geo_size = state->geo_size;
        view.pieces = state->pieces;
        view.pieces_size = state->pieces_size;
        view.view3_offset = state->view3_offset;
        view.view3_columns = state->view3_columns;
        view.view3_rows = state->view3_rows;
        view.view3_base_x = state->view3_base_x;
        view.view3_y = state->view3_y;
        view.lateral_offset = r->lateral_offset;
        view.lookup_frame_residue = state->lookup_frame_residue;
        view_status = pr_u3bd_entry5_view3_wall(&view, draw, draw_ctx,
                                                 &view_report);
        r->view3_wall[iteration] = view_report.wall;
        r->calls_done += view_report.calls_done;
        if (view_status == PR_U3BD_VIEW3_WALL_LOOKUP_UNSPECIFIED)
            return PR_U3BD_VIEW3_NEXT_LOOKUP_UNSPECIFIED;
        if (view_status == PR_U3BD_VIEW3_WALL_DRAW_HOOK)
            return PR_U3BD_VIEW3_NEXT_DRAW_HOOK;
        if (view_status != PR_U3BD_VIEW3_WALL_OK)
            return PR_U3BD_VIEW3_NEXT_BOUNDS;

        /* The second view-four gate is zero. The third is nonzero and
         * stops at +0x0AD5, before its local placement call. */
        status = nibble(state, r->sample_x, r->sample_y, state->facing,
                        &r->view4_wall[iteration]);
        if (status != PR_U3BD_VIEW3_NEXT_OK) return status;
        if ((iteration == 0u && r->view4_wall[iteration] != 0u) ||
            (iteration == 1u && r->view4_wall[iteration] == 0u))
            return PR_U3BD_VIEW3_NEXT_OTHER_PATH;
    }
    return PR_U3BD_VIEW3_NEXT_OK;
}
