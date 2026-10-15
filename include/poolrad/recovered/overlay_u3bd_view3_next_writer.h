#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_VIEW3_NEXT_WRITER_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_VIEW3_NEXT_WRITER_H

#include "poolrad/recovered/overlay_u3bd_first_wall.h"

#include <stddef.h>
#include <stdint.h>

/* Reached entry-5 prefix after the first zero view-four gate at +0x0AD8.
 * Two loop updates and two view-three placements lead to the measured
 * third-iteration +0x0AD5 call. Draw effects stay behind the callback. */
typedef struct {
    uint16_t sample_x, sample_y; /* [BP-0x0D], [BP-0x0F] */
    uint16_t lateral_offset;     /* [BP-0x12] */
    uint8_t scan_index;          /* [BP-0x10]; this prefix starts at zero */
    uint8_t direction_index;     /* [BP-2] */
    uint8_t outer_selector;      /* [BP+6], view-three lookup */
    uint8_t facing;              /* [BP-1], view-four lookup */
    uint8_t map_id;
    const uint8_t *geo;
    size_t geo_size;
    const uint8_t *pieces;
    size_t pieces_size;
    const uint8_t *x_shift;      /* DS:[0x2736], signed bytes */
    const uint8_t *y_shift;      /* DS:[0x273F], signed bytes */
    size_t shift_count;
    uint8_t view3_offset;        /* DS:[0x0C8F] */
    uint8_t view3_columns;       /* DS:[0x0C99] */
    uint8_t view3_rows;          /* DS:[0x0CA3] */
    uint16_t view3_base_x;       /* DS:[0x0CB0] */
    uint8_t view3_y;             /* DS:[0x0CC4] */
    uint8_t lookup_frame_residue;
} pr_u3bd_view3_next_writer_state;

typedef enum {
    PR_U3BD_VIEW3_NEXT_OK = 0,
    PR_U3BD_VIEW3_NEXT_ARGUMENT,
    PR_U3BD_VIEW3_NEXT_BOUNDS,
    PR_U3BD_VIEW3_NEXT_OTHER_PATH,
    PR_U3BD_VIEW3_NEXT_LOOKUP_UNSPECIFIED,
    PR_U3BD_VIEW3_NEXT_DRAW_HOOK
} pr_u3bd_view3_next_writer_status;

typedef struct {
    uint16_t sample_x, sample_y, lateral_offset;
    uint8_t scan_index;
    uint8_t view3_wall[2], view4_wall[2];
    unsigned calls_done;
} pr_u3bd_view3_next_writer_report;

pr_u3bd_view3_next_writer_status pr_u3bd_entry5_view3_next_writer(
    const pr_u3bd_view3_next_writer_state *state,
    pr_u3bd_first_wall_draw draw, void *draw_ctx,
    pr_u3bd_view3_next_writer_report *report);

#endif
