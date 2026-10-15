#include "poolrad/recovered/overlay_u3bd_0211_art_join.h"
#include "poolrad/recovered/overlay_u3bd_dual_wall.h"
#include "poolrad/recovered/overlay_u3bd_view1_scan.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { SOURCE_SIZE = 0x17 + 45 * 32, MASK_SIZE = 45 * 8,
       OBJECT_SIZE = 0x17 + 21 * 168 * 4 };

typedef struct {
    uint16_t x, y;
    uint8_t item;
} tile_tuple;

typedef struct {
    pr_u40c_tile_plane_state tile;
    unsigned calls, changed;
} draw_state;

static const tile_tuple later_calls[] = {
    {8, 6, 1}, {8, 7, 13}, {9, 6, 1}, {9, 7, 13},
    {6, 5, 1}, {6, 6, 18}, {6, 7, 26}, {6, 8, 28}
};

static size_t data_at(unsigned x, unsigned y, unsigned plane)
{
    return 0x17u + ((size_t)y * 8u * 21u + x) * 4u + plane;
}

static int draw_tile(void *opaque, uint16_t x, uint16_t y, uint8_t item,
                     uint16_t one_a, uint16_t one_b)
{
    draw_state *ctx = opaque;
    pr_u40c_tile_plane_state tile = ctx->tile;
    pr_u40c_tile_plane_report report;
    if (ctx->calls >= sizeof later_calls / sizeof later_calls[0] ||
        x != later_calls[ctx->calls].x ||
        y != later_calls[ctx->calls].y ||
        item != later_calls[ctx->calls].item ||
        one_a != 1u || one_b != 1u)
        return 1;
    tile.x = (uint8_t)x;
    tile.y = (uint8_t)y;
    tile.item_id = item;
    if (pr_u40c_entry2_mode1_tile_plane(&tile, &report) !=
        PR_U40C_TILE_PLANE_OK || report.bytes_written != 32u)
        return 1;
    ctx->changed += report.bytes_changed;
    ++ctx->calls;
    return 0;
}

int main(void)
{
    uint8_t source[SOURCE_SIZE] = {0};
    uint8_t mask[MASK_SIZE] = {0};
    uint8_t object[OBJECT_SIZE] = {0};
    uint8_t pieces[15u * 156u] = {0};
    uint8_t geo[1024] = {0};
    pr_u3bd_0211_art_join_state first = {0};
    pr_u3bd_0211_art_join_report first_report;
    pr_u3bd_dual_wall_state dual = {0};
    pr_u3bd_dual_wall_report dual_report;
    pr_u3bd_view1_scan_state view1 = {0};
    pr_u3bd_view1_scan_report view1_report;
    draw_state draw = {0};
    const size_t wall14 = 13u * 156u;
    const size_t wall15 = 14u * 156u;
    const size_t marks[] = {
        data_at(7, 7, 0), data_at(8, 7, 0), data_at(9, 7, 0),
        data_at(6, 6, 1), data_at(6, 7, 2), data_at(6, 8, 3)
    };
    const uint8_t values[] = {0x11, 0x11, 0x11, 0x22, 0x44, 0x88};
    size_t at;
    unsigned i, nonzero = 0;

    source[0] = 8;
    source[2] = 1;
    source[0x11] = 32;
    source[0x13] = 1; /* synthetic nonnull auxiliary far pointer */
    source[0x17 + 12 * 32] = 0x11;     /* item 13, plane 0 */
    source[0x17 + 17 * 32 + 1] = 0x22; /* item 18, plane 1 */
    source[0x17 + 25 * 32 + 2] = 0x44; /* item 26, plane 2 */
    source[0x17 + 27 * 32 + 3] = 0x88; /* item 28, plane 3 */
    object[0] = 168;
    object[2] = 21;
    pieces[wall14] = 1;
    pieces[wall14 + 1] = 13;
    pieces[wall14 + 154] = 1;
    pieces[wall14 + 155] = 13;
    pieces[wall15 + 2] = 1;
    pieces[wall15 + 3] = 18;
    pieces[wall15 + 4] = 26;
    pieces[wall15 + 5] = 28;
    geo[0x100 + 16 * 4 + 13] = 0xf0;

    draw.tile.group0_remap = 1;
    draw.tile.flag = 1;
    draw.tile.video_state = 1;
    draw.tile.source = source;
    draw.tile.source_size = sizeof source;
    draw.tile.aux_mask = mask;
    draw.tile.aux_mask_size = sizeof mask;
    draw.tile.destination = object;
    draw.tile.destination_size = sizeof object;

    first.wall = 14;
    first.origin_x = 5;
    first.origin_y = 4;
    first.view0_columns = 1;
    first.view0_rows = 2;
    first.pieces = pieces;
    first.pieces_size = sizeof pieces;
    first.tile = draw.tile;
    assert(pr_u3bd_0211_art_join(&first, &first_report) ==
           PR_U3BD_0211_ART_OK);
    assert(first_report.tile_calls == 2u &&
           first_report.plane_bytes_written == 64u &&
           first_report.plane_bytes_changed == 1u);
    assert(object[marks[0]] == values[0] && object[marks[1]] == 0);

    dual.previous_wall = 14;
    dual.current_wall = 14;
    dual.lateral_offset = 2;
    dual.view9.offset = 154;
    dual.view9.columns = 1;
    dual.view9.rows = 2;
    dual.view9.base_x = 5;
    dual.view9.y = 4;
    dual.view0.columns = 1;
    dual.view0.rows = 2;
    dual.view0.base_x = 5;
    dual.view0.y = 4;
    dual.pieces = pieces;
    dual.pieces_size = sizeof pieces;
    assert(pr_u3bd_entry5_dual_wall(&dual, draw_tile, &draw, &dual_report) ==
           PR_U3BD_DUAL_WALL_OK);
    assert(dual_report.calls_done == 4u && draw.calls == 4u &&
           draw.changed == 2u && dual_report.previous_wall_after == 14u);
    assert(object[marks[0]] == values[0] &&
           object[marks[1]] == values[1] &&
           object[marks[2]] == values[2]);

    view1.start_x = 13;
    view1.start_y = 4;
    view1.facing = 4;
    view1.step_y = 1;
    view1.map_id = 20;
    view1.geo = geo;
    view1.geo_size = sizeof geo;
    view1.pieces = pieces;
    view1.pieces_size = sizeof pieces;
    view1.view1_offset = 2;
    view1.view1_columns = 1;
    view1.view1_rows = 4;
    view1.view1_base_x = 4;
    view1.view1_y = 3;
    assert(pr_u3bd_entry5_view1_scan(&view1, draw_tile, &draw,
                                     &view1_report) == PR_U3BD_VIEW1_SCAN_OK);
    assert(view1_report.samples_done == 3u &&
           view1_report.calls_done == 4u &&
           draw.calls == 8u && draw.changed == 5u);
    for (i = 0; i < sizeof marks / sizeof marks[0]; ++i)
        assert(object[marks[i]] == values[i]);
    for (at = 0x17u; at < sizeof object; ++at)
        nonzero += object[at] != 0u;
    assert(nonzero == 6u);

    puts("U3BD first-view ten-tile object chain tests passed");
    return 0;
}
