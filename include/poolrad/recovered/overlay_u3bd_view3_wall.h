#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_VIEW3_WALL_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_VIEW3_WALL_H

#include "poolrad/recovered/overlay_u3bd_first_wall.h"

#include <stddef.h>
#include <stdint.h>

/* Bounded entry-5 branch +0x0A7A..+0x0AA9 and its already documented
 * local +0x0211 placement. The caller supplies the current frame words;
 * this is not a translation of the entry-5 setup or later view branches. */
typedef struct {
    uint16_t sample_x, sample_y;  /* [BP-0x0D], [BP-0x0F]; low bytes pushed */
    uint8_t selector;             /* [BP+6], not the facing byte [BP-1] */
    uint8_t map_id;
    const uint8_t *geo;
    size_t geo_size;
    const uint8_t *pieces;
    size_t pieces_size;
    uint8_t view3_offset;        /* DS:[0x0C8F] */
    uint8_t view3_columns;       /* DS:[0x0C99] */
    uint8_t view3_rows;          /* DS:[0x0CA3] */
    uint16_t view3_base_x;       /* DS:[0x0CB0] */
    uint8_t view3_y;             /* DS:[0x0CC4] */
    uint16_t lateral_offset;     /* [BP-0x12] */
    uint8_t lookup_frame_residue;
} pr_u3bd_view3_wall_state;

typedef enum {
    PR_U3BD_VIEW3_WALL_OK = 0,
    PR_U3BD_VIEW3_WALL_ARGUMENT,
    PR_U3BD_VIEW3_WALL_BOUNDS,
    PR_U3BD_VIEW3_WALL_LOOKUP_UNSPECIFIED,
    PR_U3BD_VIEW3_WALL_DRAW_HOOK
} pr_u3bd_view3_wall_status;

typedef struct {
    uint8_t wall;
    uint8_t entered_draw;
    int16_t origin_x, origin_y;
    unsigned cells_seen;
    unsigned calls_done;
} pr_u3bd_view3_wall_report;

pr_u3bd_view3_wall_status pr_u3bd_entry5_view3_wall(
    const pr_u3bd_view3_wall_state *state, pr_u3bd_first_wall_draw draw,
    void *draw_ctx, pr_u3bd_view3_wall_report *report);

#endif
