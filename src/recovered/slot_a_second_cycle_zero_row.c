#include "poolrad/recovered/slot_a_second_cycle_zero_row.h"

#include <string.h>

pr_slot_a_second_zero_row_status pr_slot_a_second_zero_row_apply(
    uint8_t *page[4], size_t page_base, size_t page_bytes,
    uint8_t row, uint8_t left, uint8_t right, uint8_t color,
    pr_slot_a_second_zero_row_report *report) {
    size_t plane, scanline, at;
    const size_t width = 22u;

    if (report == NULL) {
        return PR_SLOT_A_SECOND_ZERO_ROW_ARGUMENT;
    }
    *report = (pr_slot_a_second_zero_row_report){0};
    if (page == NULL) {
        return PR_SLOT_A_SECOND_ZERO_ROW_ARGUMENT;
    }
    for (plane = 0u; plane < 4u; ++plane) {
        if (page[plane] == NULL) {
            return PR_SLOT_A_SECOND_ZERO_ROW_ARGUMENT;
        }
    }
    if (row < 5u || row > 9u || left != 17u || right != 38u ||
        color != 0u) {
        return PR_SLOT_A_SECOND_ZERO_ROW_BOUNDARY;
    }
    for (scanline = (size_t)row * 8u;
         scanline < ((size_t)row + 1u) * 8u; ++scanline) {
        at = scanline * 40u + (size_t)left;
        if (at < page_base || at - page_base > page_bytes ||
            width > page_bytes - (at - page_base)) {
            return PR_SLOT_A_SECOND_ZERO_ROW_BOUNDARY;
        }
    }
    for (plane = 0u; plane < 4u; ++plane) {
        for (scanline = (size_t)row * 8u;
             scanline < ((size_t)row + 1u) * 8u; ++scanline) {
            size_t offset = scanline * 40u + (size_t)left - page_base;
            size_t column;
            for (column = 0u; column < width; ++column) {
                report->changed_plane_bytes +=
                    page[plane][offset + column] != 0u ? 1u : 0u;
            }
            memset(page[plane] + offset, 0, width);
            report->plane_stores += width;
        }
    }
    return PR_SLOT_A_SECOND_ZERO_ROW_OK;
}
