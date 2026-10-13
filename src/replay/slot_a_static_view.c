#include "poolrad/replay/slot_a_static_view.h"

#include "poolrad/formats/geo.h"

#include <string.h>

pr_slot_a_view_status pr_slot_a_view_prepare(
    const pr_slot_a_native_state *state, const uint8_t *geo_data,
    size_t geo_size, uint8_t geo_id, pr_slot_a_view_context *out) {
    pr_slot_a_view_context result;
    pr_geo_map map;
    size_t direction;

    if (!state || !geo_data || !out) return PR_SLOT_A_VIEW_ARGUMENT;
    if (state->map_id != geo_id) return PR_SLOT_A_VIEW_MAP_ID;
    if (state->party[0] >= PR_GEO_WIDTH || state->party[1] >= PR_GEO_WIDTH)
        return PR_SLOT_A_VIEW_POSITION;
    if (pr_geo_parse(geo_data, geo_size, &map)) return PR_SLOT_A_VIEW_GEO;
    memset(&result, 0, sizeof result);
    if (pr_slot_a_native_read_snapshot(state, &result.saved) !=
        PR_SLOT_A_NATIVE_OK) return PR_SLOT_A_VIEW_ARGUMENT;
    result.map_id = state->map_id;
    for (direction = 0; direction < 4u; ++direction) {
        if (pr_geo_wall(&map, result.saved.x, result.saved.y,
                        (pr_geo_direction)direction,
                        &result.wall[direction]) ||
            pr_geo_door(&map, result.saved.x, result.saved.y,
                        (pr_geo_direction)direction,
                        &result.door[direction])) return PR_SLOT_A_VIEW_GEO;
    }
    if (pr_geo_event(&map, result.saved.x, result.saved.y, &result.event))
        return PR_SLOT_A_VIEW_GEO;
    *out = result;
    return PR_SLOT_A_VIEW_OK;
}

void pr_slot_a_view_clear(pr_slot_a_view_frame *frame) {
    if (!frame) return;
    memset(frame, 0, sizeof *frame);
}

pr_slot_a_view_status pr_slot_a_view_blit_tile(
    pr_slot_a_view_frame *frame, const pr_ega_block *image, size_t item,
    size_t x, size_t y, int transparent_zero) {
    uint8_t tile[64];
    size_t written = 0, row, column;
    pr_ega_status status;

    if (!frame || !image) return PR_SLOT_A_VIEW_ARGUMENT;
    if (x > PR_SLOT_A_VIEW_WIDTH - 8u ||
        y > PR_SLOT_A_VIEW_HEIGHT - 8u) return PR_SLOT_A_VIEW_BOUNDS;
    if (image->height != 8u || image->width_pixels != 8u)
        return PR_SLOT_A_VIEW_IMAGE;
    if (item >= image->item_count) return PR_SLOT_A_VIEW_ITEM;
    status = pr_ega_decode_item(image, item, tile, sizeof tile, &written);
    if (status != PR_EGA_OK || written != sizeof tile)
        return PR_SLOT_A_VIEW_IMAGE;
    for (row = 0; row < 8u; ++row) {
        for (column = 0; column < 8u; ++column) {
            size_t source = row * 8u + column;
            size_t target = (y + row) * PR_SLOT_A_VIEW_WIDTH + x + column;
            if (transparent_zero && tile[source] == 0u) continue;
            frame->index[target] = tile[source];
            frame->covered[target] = 1u;
        }
    }
    return PR_SLOT_A_VIEW_OK;
}

size_t pr_slot_a_view_covered(const pr_slot_a_view_frame *frame) {
    size_t i, count = 0;
    if (!frame) return 0;
    for (i = 0; i < PR_SLOT_A_VIEW_PIXELS; ++i)
        count += frame->covered[i] != 0u;
    return count;
}
