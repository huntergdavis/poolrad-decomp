#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_VIEW4_WALL_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_VIEW4_WALL_H

#include "poolrad/recovered/overlay_u3bd_first_wall.h"

#include <stddef.h>
#include <stdint.h>

/* Entry-5 frame at +0x0AA9. This models only the +0x0AA9..+0x0AD8
 * nibble gate and its call through the established local +0x0211 walker.
 * Other scan iterations and downstream tile effects are separate. */
typedef struct {
    uint16_t sample_x, sample_y; /* [BP-0x0D], [BP-0x0F]; low bytes pushed */
    uint8_t facing;              /* [BP-1], the view-four nibble selector */
    uint8_t map_id;
    const uint8_t *geo;
    size_t geo_size;
    const uint8_t *pieces;
    size_t pieces_size;
    uint8_t view4_offset;        /* DS:[0x0C90] */
    uint8_t view4_columns;       /* DS:[0x0C9A] */
    uint8_t view4_rows;          /* DS:[0x0CA4] */
    uint16_t view4_base_x;       /* DS:[0x0CB2] */
    uint8_t view4_y;             /* DS:[0x0CC6] */
    uint16_t lateral_offset;     /* [BP-0x12] */
    uint8_t lookup_frame_residue;
} pr_u3bd_view4_wall_state;

typedef enum {
    PR_U3BD_VIEW4_WALL_OK = 0,
    PR_U3BD_VIEW4_WALL_ARGUMENT,
    PR_U3BD_VIEW4_WALL_BOUNDS,
    PR_U3BD_VIEW4_WALL_LOOKUP_UNSPECIFIED,
    PR_U3BD_VIEW4_WALL_DRAW_HOOK
} pr_u3bd_view4_wall_status;

typedef struct {
    uint8_t wall;
    uint8_t entered_draw;
    int16_t origin_x, origin_y;
    unsigned cells_seen;
    unsigned calls_done;
} pr_u3bd_view4_wall_report;

pr_u3bd_view4_wall_status pr_u3bd_entry5_view4_wall(
    const pr_u3bd_view4_wall_state *state, pr_u3bd_first_wall_draw draw,
    void *draw_ctx, pr_u3bd_view4_wall_report *report);

#endif
