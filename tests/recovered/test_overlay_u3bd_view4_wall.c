#include "poolrad/recovered/overlay_u3bd_view4_wall.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned count, fail_on;
    uint16_t x[16], y[16];
    uint8_t item[16];
} draws;

static int record_draw(void *ctx, uint16_t x, uint16_t y, uint8_t item,
                       uint16_t one_a, uint16_t one_b)
{
    draws *out = ctx;
    assert(out->count < 16 && one_a == 1u && one_b == 1u);
    out->x[out->count] = x;
    out->y[out->count] = y;
    out->item[out->count] = item;
    ++out->count;
    return out->count == out->fail_on;
}

int main(void)
{
    uint8_t geo[1024] = {0};
    uint8_t pieces[2340] = {0};
    pr_u3bd_view4_wall_state s = {0};
    pr_u3bd_view4_wall_report r;
    draws out = {0};
    const size_t first = 2u * 156u + 22u;
    unsigned i;

    s.sample_x = 8;
    s.sample_y = 7;
    s.facing = 4;
    s.map_id = 20;
    s.geo = geo;
    s.geo_size = sizeof geo;
    s.pieces = pieces;
    s.pieces_size = sizeof pieces;
    s.view4_offset = 22;
    s.view4_columns = 2;
    s.view4_rows = 8;
    s.view4_base_x = 2;
    s.view4_y = 1;
    geo[0x100 + 16 * 7 + 8] = 0x30;
    for (i = 0; i < 16; ++i) pieces[first + i] = (uint8_t)(i + 1u);

    assert(pr_u3bd_entry5_view4_wall(&s, record_draw, &out, &r) ==
           PR_U3BD_VIEW4_WALL_OK);
    assert(r.wall == 3 && r.entered_draw && r.origin_x == 2 &&
           r.origin_y == 1 && r.cells_seen == 16 && r.calls_done == 16);
    assert(out.count == 16);
    for (i = 0; i < 16; ++i)
        assert(out.x[i] == 4u + i % 2u && out.y[i] == 3u + i / 2u &&
               out.item[i] == i + 1u);

    /* Zero adjacent nibble skips all piece data and callbacks. */
    geo[0x100 + 16 * 7 + 8] = 0;
    s.pieces = NULL;
    assert(pr_u3bd_entry5_view4_wall(&s, NULL, NULL, &r) ==
           PR_U3BD_VIEW4_WALL_OK);
    assert(!r.wall && !r.entered_draw && !r.cells_seen);
    geo[0x100 + 16 * 7 + 8] = 0x30;

    /* A clipped origin still advances all sixteen cursor cells. */
    s.view4_base_x = 12;
    assert(pr_u3bd_entry5_view4_wall(&s, NULL, NULL, &r) ==
           PR_U3BD_VIEW4_WALL_OK);
    assert(r.cells_seen == 16 && r.calls_done == 0);
    s.view4_base_x = 2;
    s.pieces = pieces;

    /* A reached missing second item keeps the first callback. */
    s.pieces_size = first + 1;
    memset(&out, 0, sizeof out);
    assert(pr_u3bd_entry5_view4_wall(&s, record_draw, &out, &r) ==
           PR_U3BD_VIEW4_WALL_BOUNDS);
    assert(out.count == 1 && r.cells_seen == 2 && r.calls_done == 1);
    s.pieces_size = sizeof pieces;
    assert(pr_u3bd_entry5_view4_wall(&s, NULL, NULL, &r) ==
           PR_U3BD_VIEW4_WALL_DRAW_HOOK);
    assert(r.cells_seen == 1 && r.calls_done == 0);

    memset(&out, 0, sizeof out);
    out.fail_on = 3;
    assert(pr_u3bd_entry5_view4_wall(&s, record_draw, &out, &r) ==
           PR_U3BD_VIEW4_WALL_DRAW_HOOK);
    assert(out.count == 3 && r.cells_seen == 3 && r.calls_done == 2);

    /* Unsupported selector returns the entry-2 frame residue explicitly. */
    s.facing = 1;
    s.lookup_frame_residue = 12;
    assert(pr_u3bd_entry5_view4_wall(&s, NULL, NULL, &r) ==
           PR_U3BD_VIEW4_WALL_LOOKUP_UNSPECIFIED);
    assert(r.wall == 12 && !r.entered_draw);
    s.facing = 4;

    /* The original passes AL from each sample word, ignoring its high byte. */
    s.sample_x = 264;
    memset(&out, 0, sizeof out);
    out.fail_on = 1;
    assert(pr_u3bd_entry5_view4_wall(&s, record_draw, &out, &r) ==
           PR_U3BD_VIEW4_WALL_DRAW_HOOK);
    assert(r.wall == 3 && out.count == 1);
    s.sample_x = 8;

    assert(pr_u3bd_entry5_view4_wall(NULL, NULL, NULL, &r) ==
           PR_U3BD_VIEW4_WALL_ARGUMENT);
    s.geo_size = 511;
    assert(pr_u3bd_entry5_view4_wall(&s, NULL, NULL, &r) ==
           PR_U3BD_VIEW4_WALL_BOUNDS);
    puts("unit 0x3bd view-four placement tests passed");
    return 0;
}
