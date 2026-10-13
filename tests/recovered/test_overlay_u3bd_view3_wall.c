#include "poolrad/recovered/overlay_u3bd_view3_wall.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned count;
    uint16_t x[8], y[8];
    uint8_t item[8];
    unsigned fail_on;
} draws;

static int record_draw(void *ctx, uint16_t x, uint16_t y, uint8_t item,
                       uint16_t one_a, uint16_t one_b)
{
    draws *out = ctx;
    assert(out->count < 8 && one_a == 1u && one_b == 1u);
    out->x[out->count] = x;
    out->y[out->count] = y;
    out->item[out->count] = item;
    ++out->count;
    return out->fail_on == out->count;
}

int main(void)
{
    uint8_t geo[1024] = {0};
    uint8_t pieces[2340] = {0};
    pr_u3bd_view3_wall_state s = {0};
    pr_u3bd_view3_wall_report r;
    draws out = {0};
    const size_t first = 4u * 156u + 10u;
    const uint8_t synthetic_items[] = {7u, 8u, 9u, 10u};
    unsigned i;

    s.sample_x = 14;
    s.sample_y = 6;
    s.selector = 6; /* the outer entry-5 argument, distinct from facing 4 */
    s.map_id = 20;
    s.geo = geo;
    s.geo_size = sizeof geo;
    s.pieces = pieces;
    s.pieces_size = sizeof pieces;
    s.view3_offset = 10;
    s.view3_columns = 3;
    s.view3_rows = 4;
    s.view3_base_x = 4;
    s.view3_y = 3;
    s.lateral_offset = (uint16_t)-6;
    geo[0x100 + 16 * 6 + 14] = 0x05;
    for (i = 0; i < 4; ++i) pieces[first + i * 3 + 2] = synthetic_items[i];

    assert(pr_u3bd_entry5_view3_wall(&s, record_draw, &out, &r) ==
           PR_U3BD_VIEW3_WALL_OK);
    assert(r.wall == 5 && r.entered_draw && r.origin_x == -2 &&
           r.origin_y == 3 && r.cells_seen == 12 && r.calls_done == 4);
    assert(out.count == 4);
    for (i = 0; i < 4; ++i)
        assert(out.x[i] == 2 && out.y[i] == i + 5 &&
               out.item[i] == synthetic_items[i]);

    /* A zero nibble reaches +0x0AA9 without touching the piece table. */
    geo[0x100 + 16 * 6 + 14] = 0;
    s.pieces = NULL;
    assert(pr_u3bd_entry5_view3_wall(&s, NULL, NULL, &r) ==
           PR_U3BD_VIEW3_WALL_OK);
    assert(!r.wall && !r.entered_draw && !r.cells_seen);
    geo[0x100 + 16 * 6 + 14] = 0x05;

    /* All columns clipped: cursor advances, with no piece read or hook. */
    s.view3_base_x = 2;
    assert(pr_u3bd_entry5_view3_wall(&s, NULL, NULL, &r) ==
           PR_U3BD_VIEW3_WALL_OK);
    assert(r.origin_x == -4 && r.cells_seen == 12 && !r.calls_done);
    s.view3_base_x = 4;
    s.pieces = pieces;

    /* A reached missing second piece retains the first callback. */
    s.pieces_size = first + 5;
    memset(&out, 0, sizeof out);
    assert(pr_u3bd_entry5_view3_wall(&s, record_draw, &out, &r) ==
           PR_U3BD_VIEW3_WALL_BOUNDS);
    assert(out.count == 1 && r.cells_seen == 6 && r.calls_done == 1);
    s.pieces_size = sizeof pieces;
    assert(pr_u3bd_entry5_view3_wall(&s, NULL, NULL, &r) ==
           PR_U3BD_VIEW3_WALL_DRAW_HOOK);
    assert(r.cells_seen == 3 && r.calls_done == 0);

    memset(&out, 0, sizeof out);
    out.fail_on = 2;
    assert(pr_u3bd_entry5_view3_wall(&s, record_draw, &out, &r) ==
           PR_U3BD_VIEW3_WALL_DRAW_HOOK);
    assert(out.count == 2 && r.calls_done == 1 && r.cells_seen == 6);

    /* The merged lookup reports the unwritten-frame-byte selector path. */
    s.selector = 1;
    s.lookup_frame_residue = 12;
    assert(pr_u3bd_entry5_view3_wall(&s, NULL, NULL, &r) ==
           PR_U3BD_VIEW3_WALL_LOOKUP_UNSPECIFIED);
    assert(r.wall == 12 && !r.entered_draw);
    s.selector = 6;

    /* Coordinates are narrowed at the original AL push boundary. */
    s.sample_x = 270;
    memset(&out, 0, sizeof out);
    out.fail_on = 1;
    assert(pr_u3bd_entry5_view3_wall(&s, record_draw, &out, &r) ==
           PR_U3BD_VIEW3_WALL_DRAW_HOOK);
    assert(r.wall == 5);
    s.sample_x = 14;

    assert(pr_u3bd_entry5_view3_wall(NULL, NULL, NULL, &r) ==
           PR_U3BD_VIEW3_WALL_ARGUMENT);
    s.geo_size = 511;
    assert(pr_u3bd_entry5_view3_wall(&s, NULL, NULL, &r) ==
           PR_U3BD_VIEW3_WALL_BOUNDS);
    puts("unit 0x3bd view-three placement tests passed");
    return 0;
}
