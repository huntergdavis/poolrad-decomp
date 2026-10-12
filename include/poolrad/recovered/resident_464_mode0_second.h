#ifndef POOLRAD_RECOVERED_RESIDENT_464_MODE0_SECOND_H
#define POOLRAD_RECOVERED_RESIDENT_464_MODE0_SECOND_H

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/resident_464_mode0_primary.h"

/* Bounded second primary byte of the first RES_0464:02A5 route-0 pixel.
 * Requires the first-byte continuation at +0x04CE and stops at +0x05CB,
 * before the y/x/layer continuation. */
typedef enum {
    PR_R464_MODE0_SECOND_READY_LOOPS = 0,
    PR_R464_MODE0_SECOND_ARGUMENT,
    PR_R464_MODE0_SECOND_BOUNDS,
    PR_R464_MODE0_SECOND_TABLE,
    PR_R464_MODE0_SECOND_AUX_MAPPING
} pr_r464_mode0_second_status;

typedef enum {
    PR_R464_MODE0_SECOND_STOP_NONE = 0,
    PR_R464_MODE0_SECOND_STOP_HELPER_SOURCE,
    PR_R464_MODE0_SECOND_STOP_TABLE,
    PR_R464_MODE0_SECOND_STOP_OBJECT_OR,
    PR_R464_MODE0_SECOND_STOP_AUX_READ,
    PR_R464_MODE0_SECOND_STOP_LOOP_CONTINUATION
} pr_r464_mode0_second_stop;

typedef struct {
    uint16_t source_offset; /* DOS frame WORD, advances by four on success. */
    uint16_t data_offset;   /* DOS local WORD, advances from one to two. */
    uint16_t auxiliary_index; /* Unchanged from prefix. */
    uint16_t next_offset;   /* +0x05CB on success. */
    uint8_t primary_index;  /* 4..7, remains seven after group completion. */
    unsigned helper_calls, table_reads;
    unsigned primary_or_writes, primary_and_writes;
    pr_r464_mode0_second_stop stopped_at;
} pr_r464_mode0_second_report;

/* The synthetic lookup has the same DS +0x25FA indexing as the first
 * group: 4*helper_AL + (primary_index-4). Host arrays must retain the
 * preceding prefix/first-byte snapshot and remain nonaliasing. A missing
 * reached mapping stops without rolling back earlier writes. */
pr_r464_mode0_second_status pr_r464_mode0_second_primary_byte(
    const pr_r464_mode0_report *prefix,
    const pr_r464_mode0_primary_report *first,
    pr_r464_block_object *object, pr_r464_block_object *auxiliary,
    const uint8_t *lookup, size_t lookup_size,
    pr_r464_mode0_second_report *report);

#endif
