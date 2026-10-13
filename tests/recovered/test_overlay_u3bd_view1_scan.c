#include "poolrad/recovered/overlay_u3bd_view1_scan.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned count;
    uint16_t x[8], y[8];
    uint8_t item[8];
} draws;

static int record_draw(void *ctx, uint16_t x, uint16_t y, uint8_t item,
                       uint16_t one_a, uint16_t one_b)
{
    draws *out = ctx;
    assert(out->count < 8);
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
    pr_u3bd_view1_scan_state s = {0};
    pr_u3bd_view1_scan_report r;
    draws out = {0};
    size_t first = 14u * 156u + 2u;

    s.start_x = 13;
    s.start_y = 4;
    s.facing = 4;
    s.step_y = 1;
    s.map_id = 20;
    s.geo = geo;
    s.geo_size = sizeof geo;
    s.pieces = pieces;
    s.pieces_size = sizeof pieces;
    s.view1_offset = 2;
    s.view1_columns = 1;
    s.view1_rows = 4;
    s.view1_base_x = 4;
    s.view1_y = 3;
    geo[0x100 + 16 * 4 + 13] = 0xf0;
    pieces[first] = 1;
    pieces[first + 1] = 18;

    /* The saved-shaped first sample emits two local tile handoffs. */
    assert(pr_u3bd_entry5_view1_scan(&s, record_draw, &out, &r) ==
           PR_U3BD_VIEW1_SCAN_OK);
    assert(r.samples_done == 3 && r.sample_x[0] == 13 &&
           r.sample_y[0] == 4 && r.sample_y[1] == 5 &&
           r.sample_y[2] == 6);
    assert(r.wall[0] == 15 && r.wall[1] == 0 && r.wall[2] == 0);
    assert(r.placements == 1 && r.cells_seen == 4 && r.calls_done == 2);
    assert(r.final_x == 13 && r.final_y == 7 &&
           r.final_lateral == (uint16_t)-6);
    assert(out.count == 2 && out.x[0] == 6 && out.y[0] == 5 &&
           out.item[0] == 1 && out.x[1] == 6 && out.y[1] == 6 &&
           out.item[1] == 18);

    /* Later positions choose base+lateral-1; the third is clipped
     * before its otherwise valid piece byte is read. */
    memset(geo, 0, sizeof geo);
    memset(pieces, 0, sizeof pieces);
    s.view1_rows = 1;
    geo[0x100 + 16 * 4 + 13] = 0x10;
    geo[0x100 + 16 * 5 + 13] = 0x20;
    geo[0x100 + 16 * 6 + 13] = 0x30;
    pieces[2] = 3;
    pieces[156 + 2] = 5;
    pieces[312 + 2] = 7;
    memset(&out, 0, sizeof out);
    assert(pr_u3bd_entry5_view1_scan(&s, record_draw, &out, &r) ==
           PR_U3BD_VIEW1_SCAN_OK);
    assert(r.placements == 3 && r.cells_seen == 3 && r.calls_done == 2);
    assert(out.count == 2 && out.x[0] == 6 && out.y[0] == 5 &&
           out.item[0] == 3 && out.x[1] == 3 && out.y[1] == 5 &&
           out.item[1] == 5);

    /* Three zero nibbles avoid the piece buffer and draw hook entirely. */
    memset(geo, 0, sizeof geo);
    s.pieces = NULL;
    assert(pr_u3bd_entry5_view1_scan(&s, NULL, NULL, &r) ==
           PR_U3BD_VIEW1_SCAN_OK);
    assert(r.samples_done == 3 && !r.placements && !r.cells_seen);
    s.pieces = pieces;

    /* A reached second sample reports its piece-buffer bound after the
     * first zero sample, preserving the completed-sample count. */
    geo[0x100 + 16 * 5 + 13] = 0x20;
    s.pieces_size = 156 + 2;
    assert(pr_u3bd_entry5_view1_scan(&s, record_draw, &out, &r) ==
           PR_U3BD_VIEW1_SCAN_BOUNDS);
    assert(r.samples_done == 2 && r.placements == 1 &&
           r.cells_seen == 1 && r.calls_done == 0);
    s.pieces_size = sizeof pieces;
    assert(pr_u3bd_entry5_view1_scan(&s, NULL, NULL, &r) ==
           PR_U3BD_VIEW1_SCAN_DRAW_HOOK);
    assert(r.samples_done == 2 && r.cells_seen == 1 && r.calls_done == 0);

    s.facing = 1;
    assert(pr_u3bd_entry5_view1_scan(&s, record_draw, &out, &r) ==
           PR_U3BD_VIEW1_SCAN_LOOKUP_UNSPECIFIED);
    s.facing = 4;
    s.geo_size = 511;
    assert(pr_u3bd_entry5_view1_scan(&s, record_draw, &out, &r) ==
           PR_U3BD_VIEW1_SCAN_BOUNDS);
    assert(pr_u3bd_entry5_view1_scan(NULL, record_draw, &out, &r) ==
           PR_U3BD_VIEW1_SCAN_ARGUMENT);

    puts("unit 0x3bd view-one scan tests passed");
    return 0;
}
