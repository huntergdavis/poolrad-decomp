#ifndef POOLRAD_RECOVERED_RESIDENT_464_MODE0_SINGLE_H
#define POOLRAD_RECOVERED_RESIDENT_464_MODE0_SINGLE_H

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/resident_464_mode0_loops.h"

/* Bounded composition of the four RES_0464:02A5 video-state-0 slices
 * for count=width=height=1 only. This remains one original routine
 * identity; repeated-pixel/layer paths are outside this API. */
typedef enum {
    PR_R464_MODE0_SINGLE_RETURN = 0,
    PR_R464_MODE0_SINGLE_ARGUMENT,
    PR_R464_MODE0_SINGLE_SHAPE,
    PR_R464_MODE0_SINGLE_PREFIX_STOP,
    PR_R464_MODE0_SINGLE_FIRST_STOP,
    PR_R464_MODE0_SINGLE_SECOND_STOP,
    PR_R464_MODE0_SINGLE_LOOP_STOP
} pr_r464_mode0_single_status;

typedef enum {
    PR_R464_MODE0_SINGLE_STAGE_NONE = 0,
    PR_R464_MODE0_SINGLE_STAGE_SHAPE,
    PR_R464_MODE0_SINGLE_STAGE_PREFIX,
    PR_R464_MODE0_SINGLE_STAGE_FIRST,
    PR_R464_MODE0_SINGLE_STAGE_SECOND,
    PR_R464_MODE0_SINGLE_STAGE_LOOP,
    PR_R464_MODE0_SINGLE_STAGE_RETURN
} pr_r464_mode0_single_stage;

typedef struct {
    pr_r464_mode0_single_stage stage;
    int component_status; /* Status enum for the reached component. */
    pr_r464_mode0_report prefix;
    pr_r464_mode0_primary_report first;
    pr_r464_mode0_second_report second;
    pr_r464_mode0_loop_report loop;
} pr_r464_mode0_single_report;

/* Entry assumes the parent nonzero-object and raw video-state-zero gates.
 * The singleton shape is a host scope gate checked before DOS effects,
 * not an original DOS branch. For supported shape, each component runs
 * in original order and a reached host stop preserves preceding effects.
 * Source, object, auxiliary, and tables must be distinct mapped arrays. */
pr_r464_mode0_single_status pr_r464_mode0_single_pixel(
    const uint8_t *source, size_t source_size, uint16_t source_offset,
    uint8_t raw_unused, uint8_t selector, pr_r464_block_object *object,
    pr_r464_block_object *auxiliary, const uint8_t *aux_pattern_low,
    size_t aux_pattern_size, const uint8_t *lookup, size_t lookup_size,
    pr_r464_mode0_single_report *report);

#endif
