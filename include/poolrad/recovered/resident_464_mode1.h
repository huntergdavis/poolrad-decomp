#ifndef POOLRAD_RECOVERED_RESIDENT_464_MODE1_H
#define POOLRAD_RECOVERED_RESIDENT_464_MODE1_H

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/resident_464_block_wrapper.h"

/* Bounded model of RES_0464:02A5's raw video-state-1 route. Entry assumes
 * the earlier nonzero-object and mode-1 gates have passed. Far allocations
 * are mapped to separate host arrays from relative offset zero. */
typedef enum {
    PR_R464_MODE1_OK = 0,
    PR_R464_MODE1_ARGUMENT,
    PR_R464_MODE1_BOUNDS,
    PR_R464_MODE1_AUX_MAPPING,
    PR_R464_MODE1_TABLE,
    PR_R464_MODE1_LIMIT
} pr_r464_mode1_status;

typedef enum {
    PR_R464_MODE1_STOP_NONE = 0,
    PR_R464_MODE1_STOP_HEADER,
    PR_R464_MODE1_STOP_LIMIT,
    PR_R464_MODE1_STOP_MOVE,
    PR_R464_MODE1_STOP_AUX_WRITE,
    PR_R464_MODE1_STOP_TABLE,
    PR_R464_MODE1_STOP_PRIMARY_OR,
    PR_R464_MODE1_STOP_AUX_READ,
    PR_R464_MODE1_STOP_PRIMARY_AND
} pr_r464_mode1_stop;

typedef struct {
    uint16_t source_offset; /* DOS [BP+0x0E], wrapper initializes 0x11. */
    uint16_t data_offset;   /* DOS [BP-0x1C], initialized to zero. */
    uint16_t auxiliary_index;
    uint16_t x, y;
    uint8_t layer;
    size_t pixels_completed;
    unsigned move_calls, aux_writes, primary_or_writes, primary_and_writes;
    pr_r464_mode1_stop stopped_at;
} pr_r464_mode1_report;

/* table maps the private DS byte slice +0x25F6..+0x25F9; supply four
 * host bytes, never original game data in tests or Git. max_pixels is a
 * host work bound because zero width/height WORDs become 65536 iterations.
 * A host stop retains all earlier writes and frame-offset updates. */
pr_r464_mode1_status pr_r464_postprocess_mode1(
    const uint8_t *source, size_t source_size, uint16_t source_offset,
    uint8_t raw_unused, uint8_t selector, pr_r464_block_object *object,
    pr_r464_block_object *auxiliary, const uint8_t *table,
    size_t table_size, size_t max_pixels, pr_r464_mode1_report *report);

#endif
