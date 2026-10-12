#ifndef POOLRAD_RECOVERED_RESIDENT_464_MODE0_ITERATOR_H
#define POOLRAD_RECOVERED_RESIDENT_464_MODE0_ITERATOR_H

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/resident_464_mode0_loops.h"

/* Bounded route-0 pixel loop of RES_0464:02A5. This does not model the
 * parent video-state dispatch or nonzero-object gate. */
typedef enum {
    PR_R464_MODE0_ITER_RETURN = 0,
    PR_R464_MODE0_ITER_NO_WORK,
    PR_R464_MODE0_ITER_BUDGET,
    PR_R464_MODE0_ITER_ARGUMENT,
    PR_R464_MODE0_ITER_PREFIX_STOP,
    PR_R464_MODE0_ITER_FIRST_STOP,
    PR_R464_MODE0_ITER_SECOND_STOP,
    PR_R464_MODE0_ITER_LOOP_STOP
} pr_r464_mode0_iter_status;

typedef enum {
    PR_R464_MODE0_ITER_STAGE_NONE = 0,
    PR_R464_MODE0_ITER_STAGE_BUDGET,
    PR_R464_MODE0_ITER_STAGE_PREFIX,
    PR_R464_MODE0_ITER_STAGE_FIRST,
    PR_R464_MODE0_ITER_STAGE_SECOND,
    PR_R464_MODE0_ITER_STAGE_LOOP,
    PR_R464_MODE0_ITER_STAGE_RETURN
} pr_r464_mode0_iter_stage;

typedef struct {
    pr_r464_mode0_report prefix;
    pr_r464_mode0_primary_report first;
    pr_r464_mode0_second_report second;
    pr_r464_mode0_loop_report loop;
} pr_r464_mode0_pixel_report;

typedef struct {
    size_t pixel_index; /* Current or final zero-based record index. */
    size_t records_used; /* Includes a zero-count entry probe, if reached. */
    size_t completed_pixels; /* Both primary bytes and loop gate reached. */
    uint16_t next_offset; /* Valid on budget, no-work, or return. */
    pr_r464_mode0_iter_stage stage;
    int component_status; /* Status enum of the reached slice. */
} pr_r464_mode0_iter_report;

/* pixel_budget is a host execution limit, not a DOS field. The caller
 * provides at least pixel_budget report slots. A zero budget stops before
 * any DOS-modeled access at +0x02D3. Otherwise a zero object count can
 * return NO_WORK after the entry prefix. Each reached host stop retains
 * prior writes and the current pixel's partial slice reports. Arrays must
 * represent distinct, nonaliasing mapped DOS allocations. */
pr_r464_mode0_iter_status pr_r464_mode0_iterate(
    const uint8_t *source, size_t source_size, uint16_t source_offset,
    uint8_t raw_unused, uint8_t selector, pr_r464_block_object *object,
    pr_r464_block_object *auxiliary, const uint8_t *aux_pattern_low,
    size_t aux_pattern_size, const uint8_t *lookup, size_t lookup_size,
    size_t pixel_budget, pr_r464_mode0_pixel_report *pixels,
    size_t pixels_capacity, pr_r464_mode0_iter_report *report);

#endif
