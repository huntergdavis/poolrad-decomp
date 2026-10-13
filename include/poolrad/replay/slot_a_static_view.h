#ifndef POOLRAD_REPLAY_SLOT_A_STATIC_VIEW_H
#define POOLRAD_REPLAY_SLOT_A_STATIC_VIEW_H

#include "poolrad/formats/ega.h"
#include "poolrad/replay/slot_a_native_save.h"

#include <stddef.h>
#include <stdint.h>

#define PR_SLOT_A_VIEW_WIDTH 320u
#define PR_SLOT_A_VIEW_HEIGHT 200u
#define PR_SLOT_A_VIEW_PIXELS (PR_SLOT_A_VIEW_WIDTH * PR_SLOT_A_VIEW_HEIGHT)

/* Unknown pixels remain uncovered. The caller supplies drawing decisions;
 * this layer does not infer a DOS wall face, palette, or text glyph. */
typedef struct {
    uint8_t index[PR_SLOT_A_VIEW_PIXELS];
    uint8_t covered[PR_SLOT_A_VIEW_PIXELS];
} pr_slot_a_view_frame;

typedef struct {
    pr_slot_a_native_snapshot saved;
    uint8_t map_id;
    uint8_t wall[4]; /* GEO north, east, south, west order. */
    uint8_t door[4];
    uint8_t event;
} pr_slot_a_view_context;

typedef enum {
    PR_SLOT_A_VIEW_OK = 0,
    PR_SLOT_A_VIEW_ARGUMENT,
    PR_SLOT_A_VIEW_GEO,
    PR_SLOT_A_VIEW_MAP_ID,
    PR_SLOT_A_VIEW_POSITION,
    PR_SLOT_A_VIEW_IMAGE,
    PR_SLOT_A_VIEW_ITEM,
    PR_SLOT_A_VIEW_BOUNDS
} pr_slot_a_view_status;

/* Bind a restored world save to an already unpacked GEO block. geo_id is
 * the DAX directory ID supplied by the caller and must match the save. */
pr_slot_a_view_status pr_slot_a_view_prepare(
    const pr_slot_a_native_state *state, const uint8_t *geo_data,
    size_t geo_size, uint8_t geo_id, pr_slot_a_view_context *out);

void pr_slot_a_view_clear(pr_slot_a_view_frame *frame);

/* Draw one validated 8x8 EGA item into a 320x200 index frame. If
 * transparent_zero is true, source index zero leaves the destination and
 * its coverage unchanged. An error leaves the frame unchanged. */
pr_slot_a_view_status pr_slot_a_view_blit_tile(
    pr_slot_a_view_frame *frame, const pr_ega_block *image, size_t item,
    size_t x, size_t y, int transparent_zero);

size_t pr_slot_a_view_covered(const pr_slot_a_view_frame *frame);

#endif
