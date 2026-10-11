#include "poolrad/formats/dax.h"
#include "poolrad/formats/walldef.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    size_t archives, blocks, wallsets, slices, validated_view_offsets;
} totals;

static int audit_one(const char *path, totals *sum) {
    FILE *file = fopen(path, "rb");
    uint8_t *bytes = NULL, *decoded = NULL;
    pr_dax_archive archive;
    long length;
    size_t i;
    int ok = 0;
    if (!file) { perror(path); return -1; }
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 ||
        (unsigned long)length > PR_DAX_MAX_FILE || fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "%s: invalid file size\n", path);
        goto done;
    }
    bytes = malloc((size_t)length ? (size_t)length : 1);
    decoded = malloc(PR_WALLDEF_MAX_BLOCK);
    if (!bytes || !decoded || fread(bytes, 1, (size_t)length, file) != (size_t)length) {
        fprintf(stderr, "%s: read or allocation failure\n", path);
        goto done;
    }
    {
        pr_dax_status status = pr_dax_open(bytes, (size_t)length, &archive);
        if (status != PR_DAX_OK) {
            fprintf(stderr, "%s: %s\n", path, pr_dax_status_string(status));
            goto done;
        }
    }
    for (i = 0; i < archive.count; ++i) {
        pr_walldef_block block;
        size_t written = 0, wallset, slice_index, view_index;
        pr_dax_status dax = pr_dax_read(&archive, i, decoded,
                                        PR_WALLDEF_MAX_BLOCK, &written);
        if (dax != PR_DAX_OK) {
            fprintf(stderr, "%s: block %zu: %s\n", path, i,
                    pr_dax_status_string(dax));
            goto done;
        }
        if (pr_walldef_open(decoded, written, &block) != PR_WALLDEF_OK) {
            fprintf(stderr, "%s: block %zu: invalid WALLDEF size\n", path, i);
            goto done;
        }
        ++sum->blocks;
        sum->wallsets += block.wallset_count;
        for (wallset = 0; wallset < block.wallset_count; ++wallset) {
            for (slice_index = 0; slice_index < PR_WALLDEF_SLICES_PER_WALLSET;
                 ++slice_index) {
                pr_walldef_span slice;
                size_t previous_end;
                if (pr_walldef_slice_at(&block, wallset, slice_index, &slice) !=
                    PR_WALLDEF_OK || slice.end_offset > written ||
                    slice.end_offset - slice.source_offset != PR_WALLDEF_SLICE_SIZE) {
                    fprintf(stderr, "%s: block %zu: invalid slice span\n", path, i);
                    goto done;
                }
                ++sum->slices;
                previous_end = slice.source_offset;
                for (view_index = 0; view_index < PR_WALLDEF_VIEWS_PER_SLICE;
                     ++view_index) {
                    pr_walldef_view_span view;
                    if (pr_walldef_view_at(&block, wallset, slice_index, view_index,
                                           &view) != PR_WALLDEF_OK ||
                        view.bytes.source_offset != previous_end ||
                        view.bytes.end_offset > slice.end_offset ||
                        view.bytes.end_offset - view.bytes.source_offset !=
                            (size_t)view.rows * view.columns) {
                        fprintf(stderr, "%s: block %zu: invalid view span\n", path, i);
                        goto done;
                    }
                    previous_end = view.bytes.end_offset;
                    ++sum->validated_view_offsets;
                }
                if (previous_end != slice.end_offset) {
                    fprintf(stderr, "%s: block %zu: incomplete slice\n", path, i);
                    goto done;
                }
            }
        }
    }
    ++sum->archives;
    ok = 1;
done:
    free(decoded);
    free(bytes);
    fclose(file);
    return ok ? 0 : -1;
}

int main(int argc, char **argv) {
    totals sum = {0};
    int i;
    if (argc < 2) {
        fprintf(stderr, "usage: %s WALLDEF1.DAX [WALLDEF2.DAX ...]\n", argv[0]);
        return 2;
    }
    for (i = 1; i < argc; ++i)
        if (audit_one(argv[i], &sum) != 0) return 1;
    printf("archives=%zu blocks=%zu wallsets=%zu slices=%zu "
           "validated_view_offsets=%zu failures=0\n", sum.archives,
           sum.blocks, sum.wallsets, sum.slices, sum.validated_view_offsets);
    return 0;
}
