#include "poolrad/formats/walldef.h"

/* View tables adapted from Gold Box Explorer, Copyright (c) 2018 Bil Simser.
 * MIT notice: docs/formats/GoldBoxExplorer-MIT.txt. The same consecutive
 * arrays occur in DOS v1.3 START.EXE at file offsets 0xCE5A, 0xCE64, 0xCE6E.
 * They describe byte spans and dimensions, not wall-face meanings. */
static const uint8_t view_start[PR_WALLDEF_VIEWS_PER_SLICE] = {
    0, 2, 6, 10, 22, 38, 54, 110, 132, 154
};
static const uint8_t view_columns[PR_WALLDEF_VIEWS_PER_SLICE] = {
    1, 1, 1, 3, 2, 2, 7, 2, 2, 1
};
static const uint8_t view_rows[PR_WALLDEF_VIEWS_PER_SLICE] = {
    2, 4, 4, 4, 8, 8, 8, 11, 11, 2
};

static int valid_block(const pr_walldef_block *block) {
    return block && block->data &&
           block->wallset_count >= 1 &&
           block->wallset_count <= PR_WALLDEF_MAX_WALLSETS &&
           block->size == block->wallset_count * PR_WALLDEF_WALLSET_SIZE;
}

pr_walldef_status pr_walldef_open(const uint8_t *data, size_t size,
                                  pr_walldef_block *out) {
    pr_walldef_block result;
    if (!data || !out) return PR_WALLDEF_ARGUMENT;
    if (size < PR_WALLDEF_WALLSET_SIZE || size > PR_WALLDEF_MAX_BLOCK ||
        size % PR_WALLDEF_WALLSET_SIZE != 0) return PR_WALLDEF_SIZE;
    result.data = data;
    result.size = size;
    result.wallset_count = size / PR_WALLDEF_WALLSET_SIZE;
    *out = result;
    return PR_WALLDEF_OK;
}

pr_walldef_status pr_walldef_slice_at(const pr_walldef_block *block,
                                      size_t wallset_index, size_t slice_index,
                                      pr_walldef_span *out) {
    pr_walldef_span result;
    if (!valid_block(block) || !out) return PR_WALLDEF_ARGUMENT;
    if (wallset_index >= block->wallset_count) return PR_WALLDEF_WALLSET;
    if (slice_index >= PR_WALLDEF_SLICES_PER_WALLSET) return PR_WALLDEF_SLICE;
    result.source_offset = wallset_index * PR_WALLDEF_WALLSET_SIZE +
                           slice_index * PR_WALLDEF_SLICE_SIZE;
    result.end_offset = result.source_offset + PR_WALLDEF_SLICE_SIZE;
    *out = result;
    return PR_WALLDEF_OK;
}

pr_walldef_status pr_walldef_view_at(const pr_walldef_block *block,
                                     size_t wallset_index, size_t slice_index,
                                     size_t view_index, pr_walldef_view_span *out) {
    pr_walldef_view_span result;
    pr_walldef_span slice;
    pr_walldef_status status;
    if (!out) return PR_WALLDEF_ARGUMENT;
    status = pr_walldef_slice_at(block, wallset_index, slice_index, &slice);
    if (status != PR_WALLDEF_OK) return status;
    if (view_index >= PR_WALLDEF_VIEWS_PER_SLICE) return PR_WALLDEF_VIEW;
    result.bytes.source_offset = slice.source_offset + view_start[view_index];
    result.bytes.end_offset = slice.source_offset +
        (view_index + 1 < PR_WALLDEF_VIEWS_PER_SLICE ?
         view_start[view_index + 1] : PR_WALLDEF_SLICE_SIZE);
    result.rows = view_rows[view_index];
    result.columns = view_columns[view_index];
    if (result.bytes.end_offset < result.bytes.source_offset ||
        result.bytes.end_offset > slice.end_offset ||
        result.bytes.end_offset - result.bytes.source_offset !=
            (size_t)result.rows * result.columns) return PR_WALLDEF_LAYOUT;
    *out = result;
    return PR_WALLDEF_OK;
}

const char *pr_walldef_status_string(pr_walldef_status status) {
    switch (status) {
    case PR_WALLDEF_OK: return "ok";
    case PR_WALLDEF_ARGUMENT: return "invalid argument or block";
    case PR_WALLDEF_SIZE: return "invalid wall definition block size";
    case PR_WALLDEF_WALLSET: return "wallset index outside block";
    case PR_WALLDEF_SLICE: return "slice index outside wallset";
    case PR_WALLDEF_VIEW: return "view index outside slice";
    case PR_WALLDEF_LAYOUT: return "invalid view layout";
    default: return "unknown error";
    }
}
