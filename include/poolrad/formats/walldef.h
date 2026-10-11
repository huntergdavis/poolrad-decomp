#ifndef POOLRAD_FORMATS_WALLDEF_H
#define POOLRAD_FORMATS_WALLDEF_H

#include <stddef.h>
#include <stdint.h>

#define PR_WALLDEF_WALLSET_SIZE 780u
#define PR_WALLDEF_SLICE_SIZE 156u
#define PR_WALLDEF_SLICES_PER_WALLSET 5u
#define PR_WALLDEF_VIEWS_PER_SLICE 10u
#define PR_WALLDEF_MAX_WALLSETS 3u
#define PR_WALLDEF_MAX_BLOCK (PR_WALLDEF_WALLSET_SIZE * PR_WALLDEF_MAX_WALLSETS)

typedef enum {
    PR_WALLDEF_OK = 0,
    PR_WALLDEF_ARGUMENT,
    PR_WALLDEF_SIZE,
    PR_WALLDEF_WALLSET,
    PR_WALLDEF_SLICE,
    PR_WALLDEF_VIEW,
    PR_WALLDEF_LAYOUT
} pr_walldef_status;

/* Borrows one already unpacked WALLDEF DAX block. Keep data alive and
 * unchanged. No visual role is assigned to a wallset, slice, or view. */
typedef struct {
    const uint8_t *data;
    size_t size;
    size_t wallset_count;
} pr_walldef_block;

/* Offsets are relative to the borrowed block; end_offset is exclusive.
 * Callers may read data[source_offset..end_offset) while the block is alive. */
typedef struct {
    size_t source_offset;
    size_t end_offset;
} pr_walldef_span;

typedef struct {
    pr_walldef_span bytes;
    uint8_t rows;
    uint8_t columns;
} pr_walldef_view_span;

pr_walldef_status pr_walldef_open(const uint8_t *data, size_t size,
                                  pr_walldef_block *out);
pr_walldef_status pr_walldef_slice_at(const pr_walldef_block *block,
                                      size_t wallset_index, size_t slice_index,
                                      pr_walldef_span *out);
pr_walldef_status pr_walldef_view_at(const pr_walldef_block *block,
                                     size_t wallset_index, size_t slice_index,
                                     size_t view_index, pr_walldef_view_span *out);
const char *pr_walldef_status_string(pr_walldef_status status);

#endif
