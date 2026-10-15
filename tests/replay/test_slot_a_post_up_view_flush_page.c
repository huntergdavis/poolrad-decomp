#include "poolrad/recovered/slot_a_post_up_view_flush_page.h"

#include <assert.h>
#include <string.h>

static uint8_t planes[4][PR_SLOT_A_POST_UP_PLANE_BYTES];
static uint8_t saved[4][PR_SLOT_A_POST_UP_PLANE_BYTES];

int main(void) {
    pr_r464_dirty_state dirty = {0};
    pr_slot_a_post_up_view_page page = {0};
    pr_r464_dirty_report report = {9, 9, 9, 9};
    uint8_t object[0x17 + 8] = {0};
    unsigned plane, column;

    dirty.row[0].dirty = 1;
    dirty.row[0].first = 0;
    dirty.row[0].last = 7;
    dirty.row[0].stage_base = 0x0Fu;
    dirty.page_base = 0x0Fu;
    dirty.vram_origin = 0x10u;
    dirty.vram_size = 2;
    dirty.gc_bit_mask = 0xFFu;
    page.plane_bytes = PR_SLOT_A_POST_UP_PLANE_BYTES;
    for (plane = 0; plane < 4; ++plane) {
        memset(planes[plane], (int)(0x80u + plane), sizeof planes[plane]);
        page.plane[plane] = planes[plane];
        dirty.map_mask[plane] = (uint8_t)(1u << plane);
        for (column = 0; column < 2; ++column)
            object[0x17u + plane + 4u * column] =
                (uint8_t)(0x20u + plane * 4u + column);
    }
    memcpy(saved, planes, sizeof saved);

    assert(pr_slot_a_post_up_view_flush_page(NULL, &page, object,
                                              sizeof object, &report) ==
           PR_SLOT_A_POST_UP_FLUSH_BLOCKED_DIRTY_FRAME);
    assert(!memcmp(planes, saved, sizeof saved));
    assert(report.dirty_rows == 9);
    assert(pr_slot_a_post_up_view_flush_page(&dirty, &page, NULL, 0,
                                              &report) ==
           PR_SLOT_A_POST_UP_FLUSH_BLOCKED_OFFSCREEN);
    assert(!memcmp(planes, saved, sizeof saved));
    assert(dirty.row[0].dirty == 1 && report.dirty_rows == 9);
    assert(pr_slot_a_post_up_view_flush_page(&dirty, &page, object,
                                              sizeof object - 1u, &report) ==
           PR_SLOT_A_POST_UP_FLUSH_TRANSFER_BOUNDARY);
    assert(!memcmp(planes, saved, sizeof saved));
    assert(dirty.row[0].dirty == 1 && report.dirty_rows == 9);

    assert(pr_slot_a_post_up_view_flush_page(&dirty, &page, object,
                                              sizeof object, &report) ==
           PR_SLOT_A_POST_UP_FLUSH_OK);
    assert(report.dirty_rows == 1 && report.transfer_bytes == 2 &&
           report.stage_plane_writes == 8 && report.page_plane_writes == 8);
    for (plane = 0; plane < 4; ++plane) {
        for (column = 0; column < 2; ++column) {
            uint8_t expected = object[0x17u + plane + 4u * column];
            assert(planes[plane][0x10u + column] == expected);
            assert(planes[plane][0x2010u + column] == expected);
        }
        assert(planes[plane][0x12u] == saved[plane][0x12u]);
        assert(planes[plane][0x2012u] == saved[plane][0x2012u]);
    }
    assert(dirty.row[0].dirty == 0 && dirty.row[0].first == 0xFFFFu &&
           dirty.row[0].last == 0);
    return 0;
}
