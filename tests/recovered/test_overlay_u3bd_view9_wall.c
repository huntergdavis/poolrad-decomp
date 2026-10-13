#include "poolrad/recovered/overlay_u3bd_view9_wall.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned count;
    uint16_t x[3], y[3];
    uint8_t item[3];
} draws;

static int record_draw(void *ctx, uint16_t x, uint16_t y, uint8_t item,
                       uint16_t one_a, uint16_t one_b)
{
    draws *out = ctx;
    assert(out->count < 3);
    assert(one_a == 1u && one_b == 1u);
    out->x[out->count] = x;
    out->y[out->count] = y;
    out->item[out->count] = item;
    ++out->count;
    return 0;
}

int main(void)
{
    uint8_t geo[1024] = {0};
    uint8_t pieces[2340] = {0};
    pr_u3bd_view9_wall_state state = {0};
    pr_u3bd_view9_wall_report report;
    draws out = {0};
    const size_t first = 13u * 156u + 154u;

    state.front_x = 13;
    state.front_y = 5;
    state.lateral_offset = UINT16_MAX - 1u; /* signed word -2 */
    state.previous_wall = 14;
    state.facing = 4;
    state.step_y = 1;
    state.map_id = 20;
    state.geo = geo;
    state.geo_size = sizeof geo;
    state.pieces = pieces;
    state.pieces_size = sizeof pieces;
    state.view9_offset = 154;
    state.view9_columns = 1;
    state.view9_rows = 2;
    state.view9_base_x = 5;
    state.view9_y = 4;
    geo[0x100 + 16 * 4 + 13] = 0xF0;
    pieces[first] = 1;
    pieces[first + 1] = 13;

    assert(pr_u3bd_entry5_view9_wall(&state, record_draw, &out, &report) ==
           PR_U3BD_VIEW9_WALL_OK);
    assert(report.sample_x == 13 && report.sample_y == 4);
    assert(report.sampled_wall == 15 && report.entered_draw);
    assert(report.origin_x == 4 && report.origin_y == 4);
    assert(report.cells_seen == 2 && report.calls_done == 2);
    assert(out.count == 2 && out.x[0] == 6 && out.y[0] == 6 &&
           out.item[0] == 1 && out.x[1] == 6 && out.y[1] == 7 &&
           out.item[1] == 13);

    /* Old-wall zero skips the lookup and both data buffers. */
    state.previous_wall = 0;
    state.geo = NULL;
    state.pieces = NULL;
    assert(pr_u3bd_entry5_view9_wall(&state, NULL, NULL, &report) ==
           PR_U3BD_VIEW9_WALL_OK);
    assert(!report.cells_seen && !report.entered_draw);
    state.previous_wall = 14;
    state.geo = geo;
    state.pieces = pieces;

    /* A zero adjacent nibble suppresses the view-nine call. */
    geo[0x100 + 16 * 4 + 13] = 0;
    state.pieces = NULL;
    assert(pr_u3bd_entry5_view9_wall(&state, NULL, NULL, &report) ==
           PR_U3BD_VIEW9_WALL_OK);
    assert(report.sampled_wall == 0 && !report.entered_draw);
    geo[0x100 + 16 * 4 + 13] = 0xF0;
    state.pieces = pieces;

    /* Clipped cells advance the cursor without reading the piece buffer. */
    state.view9_base_x = 12;
    state.pieces = NULL;
    assert(pr_u3bd_entry5_view9_wall(&state, NULL, NULL, &report) ==
           PR_U3BD_VIEW9_WALL_OK);
    assert(report.origin_x == 11 && report.cells_seen == 2 &&
           report.calls_done == 0);
    state.view9_base_x = 5;
    state.pieces = pieces;

    pieces[first] = 0;
    pieces[first + 1] = 0;
    assert(pr_u3bd_entry5_view9_wall(&state, NULL, NULL, &report) ==
           PR_U3BD_VIEW9_WALL_OK);
    assert(report.cells_seen == 2 && report.calls_done == 0);

    /* A reached second-item bounds failure retains the first callback. */
    pieces[first] = 1;
    pieces[first + 1] = 13;
    state.pieces_size = first + 1;
    memset(&out, 0, sizeof out);
    assert(pr_u3bd_entry5_view9_wall(&state, record_draw, &out, &report) ==
           PR_U3BD_VIEW9_WALL_BOUNDS);
    assert(out.count == 1 && report.cells_seen == 2 &&
           report.calls_done == 1);
    state.pieces_size = sizeof pieces;
    assert(pr_u3bd_entry5_view9_wall(&state, NULL, NULL, &report) ==
           PR_U3BD_VIEW9_WALL_DRAW_HOOK);
    assert(report.calls_done == 0 && report.cells_seen == 1);

    /* The merged lookup explicitly marks an unsupported selector residue. */
    state.facing = 1;
    state.lookup_frame_residue = 15;
    assert(pr_u3bd_entry5_view9_wall(&state, record_draw, &out, &report) ==
           PR_U3BD_VIEW9_WALL_LOOKUP_UNSPECIFIED);
    state.facing = 4;

    /* Word 256 narrows to byte zero before the nibble lookup. */
    state.front_x = 256;
    geo[0x100 + 16 * 4] = 0xF0;
    memset(&out, 0, sizeof out);
    assert(pr_u3bd_entry5_view9_wall(&state, record_draw, &out, &report) ==
           PR_U3BD_VIEW9_WALL_OK);
    assert(report.sample_x == 256 && out.count == 2);

    assert(pr_u3bd_entry5_view9_wall(NULL, NULL, NULL, &report) ==
           PR_U3BD_VIEW9_WALL_ARGUMENT);
    state.geo_size = 511;
    assert(pr_u3bd_entry5_view9_wall(&state, record_draw, &out, &report) ==
           PR_U3BD_VIEW9_WALL_BOUNDS);
    puts("unit 0x3bd view-nine old-wall tests passed");
    return 0;
}
