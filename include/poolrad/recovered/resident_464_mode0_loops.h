#ifndef POOLRAD_RECOVERED_RESIDENT_464_MODE0_LOOPS_H
#define POOLRAD_RECOVERED_RESIDENT_464_MODE0_LOOPS_H

#include <stdint.h>

#include "poolrad/recovered/resident_464_mode0_second.h"

/* First-pixel y/x/layer continuation of RES_0464:02A5 route 0.
 * Entry is +0x05CB after both primary-byte groups. A next-pixel result
 * stops before the next four-byte Move at +0x0328. */
typedef enum {
    PR_R464_MODE0_LOOP_NEXT_PIXEL = 0,
    PR_R464_MODE0_LOOP_RETURN,
    PR_R464_MODE0_LOOP_ARGUMENT,
    PR_R464_MODE0_LOOP_BOUNDS
} pr_r464_mode0_loop_status;

typedef enum {
    PR_R464_MODE0_LOOP_BRANCH_NONE = 0,
    PR_R464_MODE0_LOOP_BRANCH_Y,
    PR_R464_MODE0_LOOP_BRANCH_X,
    PR_R464_MODE0_LOOP_BRANCH_LAYER,
    PR_R464_MODE0_LOOP_BRANCH_RETURN
} pr_r464_mode0_loop_branch;

typedef enum {
    PR_R464_MODE0_LOOP_STOP_NONE = 0,
    PR_R464_MODE0_LOOP_STOP_WIDTH_RELOAD,
    PR_R464_MODE0_LOOP_STOP_HEIGHT_RELOAD,
    PR_R464_MODE0_LOOP_STOP_NEXT_MOVE,
    PR_R464_MODE0_LOOP_STOP_COMMON_RETURN
} pr_r464_mode0_loop_stop;

typedef struct {
    uint16_t source_offset, data_offset, auxiliary_index;
    uint16_t width_end, height_end, x, y;
    uint16_t next_offset; /* +0x0328 or common epilogue +0x08CE. */
    uint8_t layer, count;
    pr_r464_mode0_loop_branch branch;
    pr_r464_mode0_loop_stop stopped_at;
} pr_r464_mode0_loop_report;

/* prefix holds the entry count and first-pixel dimensions; second holds
 * the completed first-pixel offsets. The object header is reread only
 * on x/layer backedges, as in DOS. Missing reached header bytes stop
 * after the preceding local increment, preserving report state. */
pr_r464_mode0_loop_status pr_r464_mode0_after_first_pixel(
    const pr_r464_mode0_report *prefix,
    const pr_r464_mode0_second_report *second,
    const pr_r464_block_object *object,
    pr_r464_mode0_loop_report *report);

#endif
