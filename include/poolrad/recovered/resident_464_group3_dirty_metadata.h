#ifndef POOLRAD_RECOVERED_RESIDENT_464_GROUP3_DIRTY_METADATA_H
#define POOLRAD_RECOVERED_RESIDENT_464_GROUP3_DIRTY_METADATA_H

#include <stddef.h>
#include <stdint.h>

/* RES_0464:08D4, reached video-state-one auxiliary-present branch.
 * source/destination include their 0x17-byte object headers. ds points to
 * a caller-owned copy of the original data segment, indexed by DOS offset.
 * Only dirty flags and the three extent arrays are modified. */
typedef struct {
    uint16_t x, y;
    uint8_t flag, video_state;
    const uint8_t *source;
    size_t source_size;
    const uint8_t *destination;
    size_t destination_size;
    uint8_t *ds;
    size_t ds_size;
} pr_464_group3_dirty_state;

typedef enum {
    PR_464_GROUP3_DIRTY_OK = 0,
    PR_464_GROUP3_DIRTY_ARGUMENT,
    PR_464_GROUP3_DIRTY_OTHER_PATH,
    PR_464_GROUP3_DIRTY_BOUNDS
} pr_464_group3_dirty_status;

typedef struct {
    unsigned rows_marked;
    unsigned first_extent_updates;
    unsigned last_extent_updates;
    unsigned changed_bytes;
} pr_464_group3_dirty_report;

/* Covers the eight-row, one-column source and 168x21 destination observed
 * at saved slot A. The original has no host buffer bounds checks. */
pr_464_group3_dirty_status pr_464_group3_dirty_metadata(
    const pr_464_group3_dirty_state *state, pr_464_group3_dirty_report *report);

#endif
