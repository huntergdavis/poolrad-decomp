#ifndef POOLRAD_RECOVERED_SLOT_A_PRINTER_PAGE_H
#define POOLRAD_RECOVERED_SLOT_A_PRINTER_PAGE_H

#include "poolrad/recovered/font_8x8d_201.h"

#include <stddef.h>
#include <stdint.h>

typedef enum {
    PR_SLOT_A_PRINTER_PAGE_OK = 0,
    PR_SLOT_A_PRINTER_PAGE_ARGUMENT,
    PR_SLOT_A_PRINTER_PAGE_BOUNDS,
    PR_SLOT_A_PRINTER_PAGE_MODEL
} pr_slot_a_printer_page_status;

typedef struct {
    size_t cells;
    size_t plane_stores;
    size_t changed_plane_bytes;
} pr_slot_a_printer_page_report;

/* Compose one reached EGA string-print call onto four supplied page planes.
 * page[p][0] represents video byte page_base; each plane has page_bytes.
 * This bounded route requires row <= 24, column <= 39, at most 255 chars,
 * all cells within the same 40-byte scanline, and all target bytes in the
 * supplied page span. Other printer paths return BOUNDS without changing
 * the page. It models page bytes only, not the printer's temporary stack. */
pr_slot_a_printer_page_status pr_slot_a_printer_page_apply(
    uint8_t *page[4], size_t page_base, size_t page_bytes,
    const pr_font201 *font, uint8_t column, uint8_t row, uint8_t color,
    const char *text, size_t length, pr_slot_a_printer_page_report *report);

#endif
