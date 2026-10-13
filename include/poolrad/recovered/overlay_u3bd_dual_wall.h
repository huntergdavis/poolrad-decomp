#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_DUAL_WALL_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_DUAL_WALL_H

#include "poolrad/recovered/overlay_u3bd_first_wall.h"

#include <stddef.h>
#include <stdint.h>

/* One local +0x0211 table/view, supplied from the original DS tables. */
typedef struct {
    uint8_t offset, columns, rows;
    uint16_t base_x;
    uint8_t y;
} pr_u3bd_dual_wall_view;

/* Entry-5 +0x0824 frame state, after its current-wall nibble lookup. */
typedef struct {
    uint8_t current_wall;       /* [BP-0x14] */
    uint8_t previous_wall;      /* [BP-0x17] */
    uint16_t lateral_offset;    /* [BP-0x12] */
    pr_u3bd_dual_wall_view view9; /* DS:[0x0C8C/0x0C96/0x0CA0+9], +0x0CBC/+0x0CD0 */
    pr_u3bd_dual_wall_view view0; /* DS:[0x0C8C/0x0C96/0x0CA0], +0x0CAA/+0x0CBE */
    const uint8_t *pieces;      /* far [0x6A58] */
    size_t pieces_size;
} pr_u3bd_dual_wall_state;

typedef enum {
    PR_U3BD_DUAL_WALL_OK = 0,
    PR_U3BD_DUAL_WALL_ARGUMENT,
    PR_U3BD_DUAL_WALL_BOUNDS,
    PR_U3BD_DUAL_WALL_OTHER_PATH,
    PR_U3BD_DUAL_WALL_DRAW_HOOK
} pr_u3bd_dual_wall_status;

typedef struct {
    uint8_t previous_wall_after;
    unsigned view9_entered, view0_entered;
    unsigned cells_seen, calls_done;
} pr_u3bd_dual_wall_report;

/* Models the nonzero current-wall arm +0x0824..+0x0865. If the previous
 * wall is nonzero, local view 9 runs first at base_x+lateral-1. Then the
 * frame's previous-wall byte becomes current_wall and local view 0 runs
 * at base_x+lateral. A zero current wall takes +0x0865, outside this model.
 * Draw effects are delegated to the merged +0x02FB callback ABI. */
pr_u3bd_dual_wall_status pr_u3bd_entry5_dual_wall(
    const pr_u3bd_dual_wall_state *state, pr_u3bd_first_wall_draw draw,
    void *draw_ctx, pr_u3bd_dual_wall_report *report);

#endif
