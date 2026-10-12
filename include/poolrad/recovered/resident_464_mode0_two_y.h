#ifndef POOLRAD_RECOVERED_RESIDENT_464_MODE0_TWO_Y_H
#define POOLRAD_RECOVERED_RESIDENT_464_MODE0_TWO_Y_H

#include "poolrad/recovered/resident_464_mode0_loops.h"

/* Route 0 for count=1, width=1, height=2 only. This is a bounded path
 * through one y backedge of RES_0464:02A5, not a second routine identity. */
typedef enum {
    PR_R464_MODE0_TWO_Y_RETURN = 0,
    PR_R464_MODE0_TWO_Y_ARGUMENT,
    PR_R464_MODE0_TWO_Y_SHAPE,
    PR_R464_MODE0_TWO_Y_PREFIX_STOP,
    PR_R464_MODE0_TWO_Y_FIRST_STOP,
    PR_R464_MODE0_TWO_Y_SECOND_STOP,
    PR_R464_MODE0_TWO_Y_LOOP_STOP
} pr_r464_mode0_two_y_status;

typedef enum {
    PR_R464_MODE0_TWO_Y_STAGE_NONE = 0,
    PR_R464_MODE0_TWO_Y_STAGE_SHAPE,
    PR_R464_MODE0_TWO_Y_STAGE_PREFIX,
    PR_R464_MODE0_TWO_Y_STAGE_FIRST,
    PR_R464_MODE0_TWO_Y_STAGE_SECOND,
    PR_R464_MODE0_TWO_Y_STAGE_LOOP,
    PR_R464_MODE0_TWO_Y_STAGE_RETURN
} pr_r464_mode0_two_y_stage;

typedef struct {
    unsigned pixel; /* Zero or one: the reached pixel at the stop. */
    pr_r464_mode0_two_y_stage stage;
    int component_status;
    pr_r464_mode0_report prefix[2];
    pr_r464_mode0_primary_report first[2];
    pr_r464_mode0_second_report second[2];
    pr_r464_mode0_loop_report loop[2];
} pr_r464_mode0_two_y_report;

/* The shape gate precedes all DOS effects. For the supported shape,
 * arrays are distinct mappings and reached host stops retain prior writes.
 * The singleton composition remains a separate, unchanged entry. */
pr_r464_mode0_two_y_status pr_r464_mode0_two_y_pixels(
    const uint8_t *source, size_t source_size, uint16_t source_offset,
    uint8_t raw_unused, uint8_t selector, pr_r464_block_object *object,
    pr_r464_block_object *auxiliary, const uint8_t *aux_pattern_low,
    size_t aux_pattern_size, const uint8_t *lookup, size_t lookup_size,
    pr_r464_mode0_two_y_report *report);

#endif
