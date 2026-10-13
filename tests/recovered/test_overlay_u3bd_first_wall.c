#include "poolrad/recovered/overlay_u3bd_first_wall.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned count;
    uint16_t x[4], y[4];
    uint8_t item[4];
} draws;

static int record_draw(void *ctx, uint16_t x, uint16_t y, uint8_t item,
                       uint16_t one_a, uint16_t one_b)
{
    draws *out = ctx;
    assert(out->count < 4);
    assert(one_a == 1 && one_b == 1);
    out->x[out->count] = x;
    out->y[out->count] = y;
    out->item[out->count] = item;
    ++out->count;
    return 0;
}

int main(void)
{
    uint8_t geo[1024] = {0};
    uint8_t pieces[3120] = {0};
    pr_u3bd_first_wall_state state = {0};
    pr_u3bd_first_wall_report report;
    draws out = {0};

    state.party_x = 15;
    state.party_y = 4;
    state.facing = 6;
    state.map_id = 20;
    state.step_x = 255;
    state.step_y = 0;
    state.geo = geo;
    state.geo_size = sizeof geo;
    state.pieces = pieces;
    state.pieces_size = sizeof pieces;
    state.view0_offset = 0;
    state.view0_columns = 1;
    state.view0_rows = 2;
    state.view0_x = 5;
    state.view0_y = 4;
    geo[0x100 + 16 * 4 + 13] = 0x0E;
    pieces[13 * 156] = 17;
    pieces[13 * 156 + 1] = 19;

    assert(pr_u3bd_entry5_first_wall(&state, record_draw, &out, &report) ==
           PR_U3BD_FIRST_WALL_OK);
    assert(report.sample_x == 13 && report.sample_y == 4);
    assert(report.wall == 14 && report.entered_draw == 1);
    assert(report.cells_seen == 2 && report.calls_done == 2);
    assert(out.count == 2);
    assert(out.x[0] == 7 && out.y[0] == 6 && out.item[0] == 17);
    assert(out.x[1] == 7 && out.y[1] == 7 && out.item[1] == 19);

    /* The zero wall exits before the piece buffer, geometry, or hook. */
    geo[0x100 + 16 * 4 + 13] = 0;
    state.pieces = NULL;
    state.view0_columns = 0;
    memset(&out, 0, sizeof out);
    assert(pr_u3bd_entry5_first_wall(&state, NULL, NULL, &report) ==
           PR_U3BD_FIRST_WALL_OK);
    assert(report.wall == 0 && !report.entered_draw && !report.cells_seen);
    state.pieces = pieces;
    state.view0_columns = 1;
    geo[0x100 + 16 * 4 + 13] = 0x0E;

    /* Local +0x0211 clips before reading each piece byte. */
    state.view0_x = 11;
    state.pieces = NULL;
    assert(pr_u3bd_entry5_first_wall(&state, NULL, NULL, &report) ==
           PR_U3BD_FIRST_WALL_OK);
    assert(report.cells_seen == 2 && report.calls_done == 0);
    state.view0_x = 5;
    state.pieces = pieces;

    /* A zero item is read but never passed to the graphics call. */
    pieces[13 * 156] = 0;
    pieces[13 * 156 + 1] = 0;
    assert(pr_u3bd_entry5_first_wall(&state, NULL, NULL, &report) ==
           PR_U3BD_FIRST_WALL_OK);
    assert(report.cells_seen == 2 && report.calls_done == 0);

    /* The second out-of-bounds item retains the first reached draw. */
    pieces[13 * 156] = 17;
    state.pieces_size = 13 * 156 + 1;
    memset(&out, 0, sizeof out);
    assert(pr_u3bd_entry5_first_wall(&state, record_draw, &out, &report) ==
           PR_U3BD_FIRST_WALL_BOUNDS);
    assert(out.count == 1 && report.calls_done == 1 && report.cells_seen == 2);
    state.pieces_size = sizeof pieces;
    assert(pr_u3bd_entry5_first_wall(&state, NULL, NULL, &report) ==
           PR_U3BD_FIRST_WALL_DRAW_HOOK);
    assert(report.cells_seen == 1 && report.calls_done == 0);

    /* The merged nibble helper marks an unmatched facing's frame residue. */
    state.facing = 1;
    state.lookup_frame_residue = 14;
    assert(pr_u3bd_entry5_first_wall(&state, record_draw, &out, &report) ==
           PR_U3BD_FIRST_WALL_LOOKUP_UNSPECIFIED);
    state.facing = 6;

    /* Out-of-grid samples reach a second DOS lookup outside this slice. */
    state.map_id = 0;
    state.step_x = 1;
    state.pieces = NULL;
    assert(pr_u3bd_entry5_first_wall(&state, NULL, NULL, &report) ==
           PR_U3BD_FIRST_WALL_OTHER_PATH);
    assert(report.sample_x == 17 && !report.entered_draw);

    /* The word 256 narrows to byte 0 at +0x06DB. The signed-byte grid
     * leaf accepts it, and the nibble lookup receives the same byte. */
    state.party_x = 2;
    state.step_x = 127;
    state.map_id = 20;
    state.pieces = pieces;
    pieces[13 * 156] = 17;
    pieces[13 * 156 + 1] = 19;
    geo[0x100 + 16 * 4] = 0x0E;
    memset(&out, 0, sizeof out);
    assert(pr_u3bd_entry5_first_wall(&state, record_draw, &out, &report) ==
           PR_U3BD_FIRST_WALL_OK);
    assert(report.sample_x == 256 && report.sample_y == 4);
    assert(report.wall == 14 && report.calls_done == 2 && out.count == 2);

    assert(pr_u3bd_entry5_first_wall(NULL, record_draw, &out, &report) ==
           PR_U3BD_FIRST_WALL_ARGUMENT);
    state.geo_size = 511;
    assert(pr_u3bd_entry5_first_wall(&state, record_draw, &out, &report) ==
           PR_U3BD_FIRST_WALL_BOUNDS);
    puts("unit 0x3bd first-wall bounded path tests passed");
    return 0;
}
