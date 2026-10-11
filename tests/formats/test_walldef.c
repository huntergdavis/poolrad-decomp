#include "poolrad/formats/walldef.h"

#include <stdio.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static int test_sizes(void) {
    uint8_t data[PR_WALLDEF_MAX_BLOCK] = {0};
    pr_walldef_block block, saved;
    pr_walldef_span final_slice;
    pr_walldef_view_span final_view;
    static const size_t valid_sizes[] = {780, 1560, 2340};
    static const size_t invalid_sizes[] = {0, 1, 779, 781, 1559, 1561, 2339, 2341, (size_t)-1};
    size_t i;
    for (i = 0; i < sizeof valid_sizes / sizeof valid_sizes[0]; ++i) {
        CHECK(pr_walldef_open(data, valid_sizes[i], &block) == PR_WALLDEF_OK);
        CHECK(block.data == data && block.size == valid_sizes[i] &&
              block.wallset_count == i + 1);
        CHECK(pr_walldef_slice_at(&block, i, 4, &final_slice) == PR_WALLDEF_OK);
        CHECK(pr_walldef_view_at(&block, i, 4, 9, &final_view) == PR_WALLDEF_OK);
        CHECK(final_slice.end_offset == valid_sizes[i] &&
              final_view.bytes.end_offset == valid_sizes[i]);
        CHECK(pr_walldef_slice_at(&block, i + 1, 0, &final_slice) ==
              PR_WALLDEF_WALLSET);
    }
    saved = block;
    for (i = 0; i < sizeof invalid_sizes / sizeof invalid_sizes[0]; ++i) {
        CHECK(pr_walldef_open(data, invalid_sizes[i], &block) == PR_WALLDEF_SIZE);
        CHECK(block.data == saved.data && block.size == saved.size &&
              block.wallset_count == saved.wallset_count);
    }
    CHECK(pr_walldef_open(NULL, 780, &block) == PR_WALLDEF_ARGUMENT);
    CHECK(pr_walldef_open(data, 780, NULL) == PR_WALLDEF_ARGUMENT);
    return 0;
}

static int test_spans(void) {
    uint8_t data[PR_WALLDEF_MAX_BLOCK] = {0};
    pr_walldef_block block, altered;
    pr_walldef_span slice, saved_slice;
    pr_walldef_view_span view, saved_view;
    static const size_t starts[10] = {0, 2, 6, 10, 22, 38, 54, 110, 132, 154};
    static const size_t lengths[10] = {2, 4, 4, 12, 16, 16, 56, 22, 22, 2};
    static const uint8_t rows[10] = {2, 4, 4, 4, 8, 8, 8, 11, 11, 2};
    static const uint8_t columns[10] = {1, 1, 1, 3, 2, 2, 7, 2, 2, 1};
    size_t wallset, face, angle;
    CHECK(pr_walldef_open(data, sizeof data, &block) == PR_WALLDEF_OK);
    for (wallset = 0; wallset < 3; ++wallset) {
        for (face = 0; face < 5; ++face) {
            size_t base = wallset * 780 + face * 156;
            CHECK(pr_walldef_slice_at(&block, wallset, face, &slice) == PR_WALLDEF_OK);
            CHECK(slice.source_offset == base && slice.end_offset == base + 156);
            for (angle = 0; angle < 10; ++angle) {
                CHECK(pr_walldef_view_at(&block, wallset, face, angle, &view) ==
                      PR_WALLDEF_OK);
                CHECK(view.bytes.source_offset == base + starts[angle] &&
                      view.bytes.end_offset == base + starts[angle] + lengths[angle]);
                CHECK(view.rows == rows[angle] && view.columns == columns[angle]);
                CHECK(view.bytes.end_offset <= slice.end_offset);
            }
        }
    }
    CHECK(slice.end_offset == sizeof data && view.bytes.end_offset == sizeof data);
    saved_slice = slice;
    saved_view = view;
    CHECK(pr_walldef_slice_at(&block, 3, 0, &slice) == PR_WALLDEF_WALLSET);
    CHECK(pr_walldef_slice_at(&block, (size_t)-1, 0, &slice) == PR_WALLDEF_WALLSET);
    CHECK(pr_walldef_slice_at(&block, 0, 5, &slice) == PR_WALLDEF_SLICE);
    CHECK(pr_walldef_slice_at(&block, 0, (size_t)-1, &slice) == PR_WALLDEF_SLICE);
    CHECK(slice.source_offset == saved_slice.source_offset &&
          slice.end_offset == saved_slice.end_offset);
    CHECK(pr_walldef_view_at(&block, 0, 0, 10, &view) == PR_WALLDEF_VIEW);
    CHECK(pr_walldef_view_at(&block, 0, 0, (size_t)-1, &view) == PR_WALLDEF_VIEW);
    CHECK(pr_walldef_view_at(&block, 3, 0, 0, &view) == PR_WALLDEF_WALLSET);
    CHECK(pr_walldef_view_at(&block, 0, 5, 0, &view) == PR_WALLDEF_SLICE);
    CHECK(view.bytes.source_offset == saved_view.bytes.source_offset &&
          view.bytes.end_offset == saved_view.bytes.end_offset &&
          view.rows == saved_view.rows && view.columns == saved_view.columns);
    CHECK(pr_walldef_slice_at(NULL, 0, 0, &slice) == PR_WALLDEF_ARGUMENT);
    CHECK(pr_walldef_slice_at(&block, 0, 0, NULL) == PR_WALLDEF_ARGUMENT);
    CHECK(pr_walldef_view_at(NULL, 0, 0, 0, &view) == PR_WALLDEF_ARGUMENT);
    CHECK(pr_walldef_view_at(&block, 0, 0, 0, NULL) == PR_WALLDEF_ARGUMENT);
    altered = block;
    altered.wallset_count = 4;
    CHECK(pr_walldef_slice_at(&altered, 0, 0, &slice) == PR_WALLDEF_ARGUMENT);
    altered = block;
    altered.size--;
    CHECK(pr_walldef_view_at(&altered, 0, 0, 0, &view) == PR_WALLDEF_ARGUMENT);
    return 0;
}

int main(void) {
    if (test_sizes() || test_spans()) return 1;
    puts("synthetic WALLDEF size/slice/view tests passed");
    return 0;
}
