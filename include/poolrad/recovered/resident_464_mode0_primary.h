#ifndef POOLRAD_RECOVERED_RESIDENT_464_MODE0_PRIMARY_H
#define POOLRAD_RECOVERED_RESIDENT_464_MODE0_PRIMARY_H

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/resident_464_mode0_prefix.h"

/* Bounded first primary byte of RES_0464:02A5 video-state-0 route.
 * Input must be the successful first-pixel prefix report at +0x03D1.
 * This step stops before second-group index 4 at +0x04CE. */
typedef enum {
    PR_R464_MODE0_PRIMARY_READY_SECOND = 0,
    PR_R464_MODE0_PRIMARY_ARGUMENT,
    PR_R464_MODE0_PRIMARY_BOUNDS,
    PR_R464_MODE0_PRIMARY_TABLE,
    PR_R464_MODE0_PRIMARY_AUX_MAPPING
} pr_r464_mode0_primary_status;

typedef enum {
    PR_R464_MODE0_PRIMARY_STOP_NONE = 0,
    PR_R464_MODE0_PRIMARY_STOP_HELPER_SOURCE,
    PR_R464_MODE0_PRIMARY_STOP_TABLE,
    PR_R464_MODE0_PRIMARY_STOP_OBJECT_OR,
    PR_R464_MODE0_PRIMARY_STOP_AUX_READ,
    PR_R464_MODE0_PRIMARY_STOP_SECOND_GROUP
} pr_r464_mode0_primary_stop;

typedef struct {
    uint16_t source_offset; /* Still unchanged from the first pixel. */
    uint16_t data_offset;   /* Advances from zero to one on completion. */
    uint16_t auxiliary_index; /* Prefix value, normally zero or two. */
    uint16_t next_offset;   /* +0x04CE on completion. */
    uint8_t primary_index;  /* Next index: 0..4. */
    unsigned helper_calls, table_reads;
    unsigned primary_or_writes, primary_and_writes;
    pr_r464_mode0_primary_stop stopped_at;
} pr_r464_mode0_primary_report;

/* lookup[4*helper_AL + primary_index] supplies the private DS byte table
 * at +0x25FA. Values are host-provided; tests use synthetic entries only.
 * Missing reached bytes stop without rolling back earlier OR writes.
 * The prefix report, object, and auxiliary arrays must represent the same
 * nonaliasing snapshot as the preceding prefix call. */
pr_r464_mode0_primary_status pr_r464_mode0_first_primary_byte(
    const pr_r464_mode0_report *prefix, pr_r464_block_object *object,
    pr_r464_block_object *auxiliary, const uint8_t *lookup,
    size_t lookup_size, pr_r464_mode0_primary_report *report);

/* The same +0x03D1..+0x04CE instructions at a carried pixel offset. */
pr_r464_mode0_primary_status pr_r464_mode0_primary_byte(
    const pr_r464_mode0_report *prefix, pr_r464_block_object *object,
    pr_r464_block_object *auxiliary, const uint8_t *lookup,
    size_t lookup_size, pr_r464_mode0_primary_report *report);

#endif
