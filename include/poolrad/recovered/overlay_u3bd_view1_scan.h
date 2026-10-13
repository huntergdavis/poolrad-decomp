#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_VIEW1_SCAN_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_VIEW1_SCAN_H

#include "poolrad/recovered/overlay_u3bd_first_wall.h"

#include <stddef.h>
#include <stdint.h>

/* Entry 5's three-position view-one scan at +0x08E8..+0x097F.
 * All decoded GEO and piece bytes remain caller-owned. */
typedef struct {
    uint16_t start_x, start_y;       /* entry-5 [BP-5], [BP-7] */
    uint8_t facing, step_x, step_y;  /* [BP-1], DS:[0x2736/0x273F+facing] */
    uint8_t map_id, lookup_frame_residue;
    const uint8_t *geo;              /* far [0x6A5C] */
    size_t geo_size;
    const uint8_t *pieces;           /* far [0x6A58] */
    size_t pieces_size;
    uint8_t view1_offset;            /* DS:[0x0C8D] */
    uint8_t view1_columns;           /* DS:[0x0C97] */
    uint8_t view1_rows;              /* DS:[0x0CA1] */
    uint16_t view1_base_x;           /* DS:[0x0CAC] */
    uint8_t view1_y;                 /* DS:[0x0CC0] */
} pr_u3bd_view1_scan_state;

typedef enum {
    PR_U3BD_VIEW1_SCAN_OK = 0,
    PR_U3BD_VIEW1_SCAN_ARGUMENT,
    PR_U3BD_VIEW1_SCAN_BOUNDS,
    PR_U3BD_VIEW1_SCAN_LOOKUP_UNSPECIFIED,
    PR_U3BD_VIEW1_SCAN_DRAW_HOOK
} pr_u3bd_view1_scan_status;

typedef struct {
    uint16_t sample_x[3], sample_y[3];
    uint8_t wall[3];
    uint16_t final_x, final_y;       /* locals at +0x097F, after third step */
    uint16_t final_lateral;          /* [BP-0x12] at +0x097F */
    unsigned samples_done, placements, cells_seen, calls_done;
} pr_u3bd_view1_scan_report;

/* Calls draw with each reached local +0x02FB tuple (x,y,item,1,1).
 * The host owns the later 0x428:002A graphics effect. */
pr_u3bd_view1_scan_status pr_u3bd_entry5_view1_scan(
    const pr_u3bd_view1_scan_state *state, pr_u3bd_first_wall_draw draw,
    void *draw_ctx, pr_u3bd_view1_scan_report *report);

#endif
