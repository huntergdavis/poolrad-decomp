#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_VIEW9_WALL_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_VIEW9_WALL_H

#include "poolrad/recovered/overlay_u3bd_first_wall.h"

#include <stddef.h>
#include <stdint.h>

/* Bounded entry-5 continuation at +0x0744..+0x0795. The original frame
 * supplies the previous wall and front-cell coordinates. Host buffers hold
 * the already loaded GEO and piece tables; no original media is embedded. */
typedef struct {
    uint16_t front_x, front_y;       /* entry-5 [BP-0x0D], [BP-0x0F] */
    uint16_t lateral_offset;         /* [BP-0x12], added as a DOS word */
    uint8_t previous_wall;           /* [BP-0x17] */
    uint8_t facing, step_x, step_y;  /* [BP-1], DS:[0x2736/0x273F+facing] */
    uint8_t map_id, lookup_frame_residue;
    const uint8_t *geo;              /* far [0x6A5C] */
    size_t geo_size;
    const uint8_t *pieces;           /* far [0x6A58] */
    size_t pieces_size;
    uint8_t view9_offset;            /* DS:[0x0C8C+9] */
    uint8_t view9_columns;           /* DS:[0x0C96+9] */
    uint8_t view9_rows;              /* DS:[0x0CA0+9] */
    uint16_t view9_base_x;           /* DS:[0x0CBC] */
    uint8_t view9_y;                 /* DS:[0x0CD0] */
} pr_u3bd_view9_wall_state;

typedef enum {
    PR_U3BD_VIEW9_WALL_OK = 0,
    PR_U3BD_VIEW9_WALL_ARGUMENT,
    PR_U3BD_VIEW9_WALL_BOUNDS,
    PR_U3BD_VIEW9_WALL_LOOKUP_UNSPECIFIED,
    PR_U3BD_VIEW9_WALL_DRAW_HOOK
} pr_u3bd_view9_wall_status;

typedef struct {
    uint16_t sample_x, sample_y;     /* words passed to +0x047B, then narrowed */
    uint8_t sampled_wall;            /* AL from +0x047B */
    uint8_t entered_draw;            /* +0x0792 reached */
    int16_t origin_x, origin_y;      /* signed low bytes read by +0x0211 */
    unsigned cells_seen, calls_done;
} pr_u3bd_view9_wall_report;

/* Reproduces the reached old-wall gate, adjacent-cell nibble test, and
 * mode-nine local +0x0211 placement walk. `draw` receives each original
 * +0x02FB (x,y,item,1,1) handoff. Drawing effects remain in the host hook. */
pr_u3bd_view9_wall_status pr_u3bd_entry5_view9_wall(
    const pr_u3bd_view9_wall_state *state, pr_u3bd_first_wall_draw draw,
    void *draw_ctx, pr_u3bd_view9_wall_report *report);

#endif
