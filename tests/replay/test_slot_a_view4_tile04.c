#include "poolrad/recovered/overlay_u3bd_view4_wall.h"
#include "poolrad/recovered/overlay_u40c_tile_plane.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    HEADER = 0x17,
    SOURCE_BYTES = HEADER + 45 * 32,
    MASK_BYTES = 45 * 8,
    DEST_BYTES = HEADER + 168 * 21 * 4,
    FOURTH_DATA_OFFSET = (4 * 8 * 21 + 5) * 4
};

typedef struct {
    pr_u40c_tile_plane_state tile;
    uint8_t *snapshot_after_third;
    unsigned calls;
} draw_context;

static void put_word(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static int draw_tile(void *opaque, uint16_t x, uint16_t y, uint8_t item,
                     uint16_t one_a, uint16_t one_b)
{
    draw_context *ctx = opaque;
    pr_u40c_tile_plane_report report;

    assert(one_a == 1u && one_b == 1u);
    assert(ctx->calls < 4u);
    ctx->tile.x = (uint8_t)x;
    ctx->tile.y = (uint8_t)y;
    ctx->tile.item_id = item;
    assert(pr_u40c_entry2_mode1_tile_plane(&ctx->tile, &report) ==
           PR_U40C_TILE_PLANE_OK);
    ++ctx->calls;
    if (ctx->calls == 3u)
        memcpy(ctx->snapshot_after_third, ctx->tile.destination, DEST_BYTES);
    if (ctx->calls == 4u) {
        assert(x == 5u && y == 4u && item == 1u);
        assert(report.item_index == 0u);
        assert(report.first_data_offset == FOURTH_DATA_OFFSET);
        assert(report.bytes_written == 32u && report.bytes_changed == 0u);
        assert(memcmp(ctx->snapshot_after_third, ctx->tile.destination,
                      DEST_BYTES) == 0);
    }
    return 0;
}

int main(void)
{
    uint8_t geo[1024] = {0};
    uint8_t pieces[2340] = {0};
    uint8_t source[SOURCE_BYTES] = {0};
    uint8_t mask[MASK_BYTES];
    uint8_t destination[DEST_BYTES] = {0};
    uint8_t before_fourth[DEST_BYTES];
    pr_u3bd_view4_wall_state wall = {0};
    pr_u3bd_view4_wall_report wall_report;
    pr_u40c_tile_plane_report tile_report;
    draw_context ctx = {0};
    const size_t first_piece = 2u * 156u + 22u;
    size_t i;

    /* Synthetic first four placements. The third makes one object change;
     * the fourth reuses index zero with an identity mask and empty source. */
    geo[0x100u + 16u * 7u + 8u] = 0x30u;
    pieces[first_piece + 0u] = 1u;
    pieces[first_piece + 1u] = 1u;
    pieces[first_piece + 2u] = 18u;
    pieces[first_piece + 3u] = 1u;
    put_word(source, 8u);
    put_word(source + 2u, 1u);
    put_word(source + 0x11u, 32u);
    put_word(source + 0x13u, 1u);
    put_word(destination, 168u);
    put_word(destination + 2u, 21u);
    memset(destination + HEADER, 0x33, DEST_BYTES - HEADER);
    memset(mask, 0xff, sizeof mask);
    source[HEADER + 17u * 32u] = 0x0cu;
    mask[17u * 8u] = 0xf0u;

    wall.sample_x = 8u;
    wall.sample_y = 7u;
    wall.facing = 4u;
    wall.map_id = 20u;
    wall.geo = geo;
    wall.geo_size = sizeof geo;
    wall.pieces = pieces;
    wall.pieces_size = sizeof pieces;
    wall.view4_offset = 22u;
    wall.view4_columns = 2u;
    wall.view4_rows = 8u;
    wall.view4_base_x = 2u;
    wall.view4_y = 1u;

    ctx.tile.group0_remap = 1u;
    ctx.tile.flag = 1u;
    ctx.tile.video_state = 1u;
    ctx.tile.source = source;
    ctx.tile.source_size = sizeof source;
    ctx.tile.aux_mask = mask;
    ctx.tile.aux_mask_size = sizeof mask;
    ctx.tile.destination = destination;
    ctx.tile.destination_size = sizeof destination;
    ctx.snapshot_after_third = before_fourth;
    assert(pr_u3bd_entry5_view4_wall(&wall, draw_tile, &ctx, &wall_report) ==
           PR_U3BD_VIEW4_WALL_OK);
    assert(wall_report.cells_seen == 16u && wall_report.calls_done == 4u);
    assert(ctx.calls == 4u);

    /* A changed synthetic index-zero source and mask must affect only the
     * fourth tile's row-zero plane bytes. */
    mask[0] = 0xf0u;
    source[HEADER + 0u] = 0x06u;
    source[HEADER + 1u] = 0x08u;
    assert(pr_u40c_entry2_mode1_tile_plane(&ctx.tile, &tile_report) ==
           PR_U40C_TILE_PLANE_OK);
    assert(tile_report.first_data_offset == FOURTH_DATA_OFFSET);
    assert(tile_report.bytes_written == 32u && tile_report.bytes_changed == 4u);
    assert(tile_report.first_changed_offset == FOURTH_DATA_OFFSET);
    for (i = 0; i < DEST_BYTES; ++i) {
        uint8_t expected = before_fourth[i];
        if (i == HEADER + FOURTH_DATA_OFFSET) expected = 0x36u;
        if (i == HEADER + FOURTH_DATA_OFFSET + 1u) expected = 0x38u;
        if (i == HEADER + FOURTH_DATA_OFFSET + 2u ||
            i == HEADER + FOURTH_DATA_OFFSET + 3u) expected = 0x30u;
        assert(destination[i] == expected);
    }

    puts("saved view-four fourth tile composition tests passed");
    return 0;
}
