#ifndef POOLRAD_RECOVERED_RESIDENT_464_FIRST_STAGE_ROW_H
#define POOLRAD_RECOVERED_RESIDENT_464_FIRST_STAGE_ROW_H

#include "poolrad/recovered/resident_464_dirty_transfer.h"

typedef enum {
    PR_R464_FIRST_STAGE_OK = 0,
    PR_R464_FIRST_STAGE_BAD_ARG,
    PR_R464_FIRST_STAGE_OTHER_PATH,
    PR_R464_FIRST_STAGE_BOUNDS
} pr_r464_first_stage_status;

typedef struct {
    unsigned row;
    uint16_t first, last;
    size_t stage_offset;
    size_t bytes_per_plane;
    size_t plane_writes;
} pr_r464_first_stage_report;

/* Video-state-1 prefix of RES_0464:1B52. Scan for the first dirty row,
 * stage its four plane loops, and stop before the +1D27 loop decision.
 * Page planes and dirty metadata are untouched. Unsupported host input
 * returns before any staging write. */
pr_r464_first_stage_status pr_r464_first_stage_row(
    const pr_r464_dirty_state *state, uint8_t video_state,
    const uint8_t *object, size_t object_size,
    pr_r464_first_stage_report *report);

#endif
