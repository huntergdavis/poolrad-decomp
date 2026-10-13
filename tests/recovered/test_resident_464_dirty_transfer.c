#include "poolrad/recovered/resident_464_dirty_transfer.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

typedef struct {
    pr_r464_dirty_state state;
    uint8_t staging[4][80], page[4][80];
    uint8_t object[128];
} fixture;

static void setup(fixture *f) {
    unsigned p;
    memset(f, 0, sizeof *f);
    f->state.gc_bit_mask = 0xFF;
    f->state.vram_size = 80;
    memset(f->staging, 0xA5, sizeof f->staging);
    memset(f->page, 0x5A, sizeof f->page);
    for (p = 0; p < 4; ++p) {
        f->state.map_mask[p] = (uint8_t)(1u << p);
        f->state.staging[p] = f->staging[p];
        f->state.page[p] = f->page[p];
    }
    f->state.row[0].dirty = 1;
    f->state.row[0].first = 0x40;
    f->state.row[0].last = 0x47;
    f->state.row[1].dirty = 1;
    f->state.row[1].first = 0x48;
    f->state.row[1].last = 0x4F;
    f->state.row[1].stage_base = 40;
}

static int test_two_rows_and_unselected_gaps(void) {
    fixture f;
    pr_r464_dirty_report report = {0};
    unsigned y, p, j;
    setup(&f);
    for (y = 0; y < 2; ++y)
        for (p = 0; p < 4; ++p)
            for (j = 0; j < 2; ++j)
                f.object[0x17u + f.state.row[y].first + p + 4u * j] =
                    (uint8_t)(y * 50u + p * 10u + j + 1u);
    CHECK(pr_r464_dirty_transfer(&f.state, f.object, sizeof f.object,
                                 &report) == PR_R464_DIRTY_OK);
    CHECK(report.dirty_rows == 2 && report.transfer_bytes == 4);
    CHECK(report.stage_plane_writes == 16 &&
          report.page_plane_writes == 16);
    for (y = 0; y < 2; ++y)
        for (p = 0; p < 4; ++p)
            for (j = 0; j < 2; ++j) {
                uint8_t expected =
                    (uint8_t)(y * 50u + p * 10u + j + 1u);
                CHECK(f.staging[p][1u + y * 40u + j] == expected);
                CHECK(f.page[p][1u + y * 40u + j] == expected);
            }
    CHECK(f.staging[2][10] == 0xA5 && f.page[2][10] == 0x5A);
    for (y = 0; y < PR_R464_DIRTY_ROWS; ++y)
        CHECK(f.state.row[y].dirty == 0 &&
              f.state.row[y].first == 0xFFFFu &&
              f.state.row[y].last == 0);
    return 0;
}

static int test_all_staging_before_any_move(void) {
    fixture f;
    pr_r464_dirty_report report = {0};
    unsigned p, j;
    setup(&f);
    f.state.row[1].stage_base = 0; /* synthetic overlap */
    for (p = 0; p < 4; ++p)
        for (j = 0; j < 2; ++j) {
            f.object[0x17u + 0x40u + p + 4u * j] =
                (uint8_t)(p * 20u + j + 1u);
            f.object[0x17u + 0x48u + p + 4u * j] =
                (uint8_t)(p * 20u + j + 101u);
        }
    CHECK(pr_r464_dirty_transfer(&f.state, f.object, sizeof f.object,
                                 &report) == PR_R464_DIRTY_OK);
    for (p = 0; p < 4; ++p)
        for (j = 0; j < 2; ++j) {
            uint8_t later = (uint8_t)(p * 20u + j + 101u);
            CHECK(f.staging[p][1u + j] == later);
            CHECK(f.page[p][1u + j] == later);
            CHECK(f.page[p][41u + j] == later);
        }
    return 0;
}

static int test_map_masks_and_empty_flush(void) {
    fixture f;
    pr_r464_dirty_report report = {0};
    unsigned j;
    setup(&f);
    f.state.row[1].dirty = 0;
    f.state.map_mask[0] = 3;
    f.state.map_mask[1] = 0;
    f.state.map_mask[2] = 4;
    f.state.map_mask[3] = 0;
    for (j = 0; j < 2; ++j) {
        f.object[0x17u + 0x40u + 4u * j] = (uint8_t)(j + 7u);
        f.object[0x17u + 0x42u + 4u * j] = (uint8_t)(j + 80u);
    }
    CHECK(pr_r464_dirty_transfer(&f.state, f.object, sizeof f.object,
                                 &report) == PR_R464_DIRTY_OK);
    CHECK(report.dirty_rows == 1 && report.transfer_bytes == 2 &&
          report.stage_plane_writes == 6 &&
          report.page_plane_writes == 8);
    for (j = 0; j < 2; ++j) {
        CHECK(f.page[0][1u + j] == (uint8_t)(j + 7u));
        CHECK(f.page[1][1u + j] == (uint8_t)(j + 7u));
        CHECK(f.page[2][1u + j] == (uint8_t)(j + 80u));
        CHECK(f.page[3][1u + j] == 0xA5);
    }
    report.dirty_rows = 99;
    CHECK(pr_r464_dirty_transfer(&f.state, f.object, sizeof f.object,
                                 &report) == PR_R464_DIRTY_OK);
    CHECK(report.dirty_rows == 0 && report.transfer_bytes == 0 &&
          report.stage_plane_writes == 0 &&
          report.page_plane_writes == 0);
    return 0;
}

static int test_host_stops_preserve_inputs(void) {
    fixture f;
    pr_r464_dirty_state before;
    uint8_t stage_before[4][80], page_before[4][80];
    pr_r464_dirty_report report = {77, 88, 99, 111};
    setup(&f);
    before = f.state;
    memcpy(stage_before, f.staging, sizeof stage_before);
    memcpy(page_before, f.page, sizeof page_before);
    CHECK(pr_r464_dirty_transfer(&f.state, f.object, 0x17u + 0x4Fu,
                                 &report) == PR_R464_DIRTY_SHORT_OBJECT);
    CHECK(memcmp(&f.state, &before, sizeof before) == 0 &&
          memcmp(f.staging, stage_before, sizeof stage_before) == 0 &&
          memcmp(f.page, page_before, sizeof page_before) == 0 &&
          report.dirty_rows == 77);
    f.state.gc_bit_mask = 0x7F;
    CHECK(pr_r464_dirty_transfer(&f.state, f.object, sizeof f.object,
                                 &report) == PR_R464_DIRTY_UNSUPPORTED);
    f.state.gc_bit_mask = 0xFF;
    f.state.vram_size = 42;
    CHECK(pr_r464_dirty_transfer(&f.state, f.object, sizeof f.object,
                                 &report) == PR_R464_DIRTY_SHORT_VRAM);
    f.state.vram_size = 80;
    f.state.row[0].last = 0x46;
    CHECK(pr_r464_dirty_transfer(&f.state, f.object, sizeof f.object,
                                 &report) == PR_R464_DIRTY_UNSUPPORTED);
    return 0;
}

int main(void) {
    if (test_two_rows_and_unselected_gaps()) return 1;
    if (test_all_staging_before_any_move()) return 1;
    if (test_map_masks_and_empty_flush()) return 1;
    if (test_host_stops_preserve_inputs()) return 1;
    return 0;
}
