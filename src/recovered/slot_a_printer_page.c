#include "poolrad/recovered/slot_a_printer_page.h"

#include <string.h>

pr_slot_a_printer_page_status pr_slot_a_printer_page_apply(
    uint8_t *page[4], size_t page_base, size_t page_bytes,
    const pr_font201 *font, uint8_t column, uint8_t row, uint8_t color,
    const char *text, size_t length, pr_slot_a_printer_page_report *report) {
    pr_slot_a_view_frame frame;
    pr_font201_status font_status;
    size_t drawn, cell, glyph_row, plane, bit;
    if (report == NULL) {
        return PR_SLOT_A_PRINTER_PAGE_ARGUMENT;
    }
    *report = (pr_slot_a_printer_page_report){0};
    if (page == NULL || font == NULL || font->data == NULL ||
        font->size != PR_FONT201_BLOCK_SIZE || (text == NULL && length != 0u)) {
        return PR_SLOT_A_PRINTER_PAGE_ARGUMENT;
    }
    for (plane = 0u; plane < 4u; ++plane) {
        if (page[plane] == NULL) {
            return PR_SLOT_A_PRINTER_PAGE_ARGUMENT;
        }
    }
    if (column > 39u || row > 24u || length > 255u ||
        length > 40u - (size_t)column) {
        return PR_SLOT_A_PRINTER_PAGE_BOUNDS;
    }
    for (cell = 0u; cell < length; ++cell) {
        for (glyph_row = 0u; glyph_row < 8u; ++glyph_row) {
            size_t at = ((size_t)row * 8u + glyph_row) * 40u +
                        (size_t)column + cell;
            if (at < page_base || at - page_base >= page_bytes) {
                return PR_SLOT_A_PRINTER_PAGE_BOUNDS;
            }
        }
    }
    memset(&frame, 0, sizeof frame);
    font_status = pr_font201_print_ega(&frame, font, column, row, color,
                                        text, length, &drawn);
    if (font_status != PR_FONT201_OK || drawn != length) {
        return PR_SLOT_A_PRINTER_PAGE_MODEL;
    }
    /* Validate the whole cell rectangle before changing any supplied page. */
    for (cell = 0u; cell < length; ++cell) {
        for (glyph_row = 0u; glyph_row < 8u; ++glyph_row) {
            size_t pixel = ((size_t)row * 8u + glyph_row) * PR_SLOT_A_VIEW_WIDTH +
                           ((size_t)column + cell) * 8u;
            for (bit = 0u; bit < 8u; ++bit) {
                if (frame.covered[pixel + bit] != 1u ||
                    frame.index[pixel + bit] > 15u) {
                    return PR_SLOT_A_PRINTER_PAGE_MODEL;
                }
            }
        }
    }
    for (cell = 0u; cell < length; ++cell) {
        for (glyph_row = 0u; glyph_row < 8u; ++glyph_row) {
            size_t at = ((size_t)row * 8u + glyph_row) * 40u +
                        (size_t)column + cell - page_base;
            size_t pixel = ((size_t)row * 8u + glyph_row) * PR_SLOT_A_VIEW_WIDTH +
                           ((size_t)column + cell) * 8u;
            for (plane = 0u; plane < 4u; ++plane) {
                uint8_t value = 0u;
                for (bit = 0u; bit < 8u; ++bit) {
                    if ((frame.index[pixel + bit] & (uint8_t)(1u << plane)) != 0u) {
                        value = (uint8_t)(value | (uint8_t)(0x80u >> bit));
                    }
                }
                report->changed_plane_bytes += page[plane][at] != value ? 1u : 0u;
                page[plane][at] = value;
                ++report->plane_stores;
            }
        }
    }
    report->cells = length;
    return PR_SLOT_A_PRINTER_PAGE_OK;
}
