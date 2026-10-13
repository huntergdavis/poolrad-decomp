#include "poolrad/recovered/slot_a_printer_page.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

enum { PAGE_BASE = 0x3C0, PAGE_BYTES = 3520, COLUMN = 34, ROW = 5 };

int main(void) {
    uint8_t font_bytes[PR_FONT201_BLOCK_SIZE] = {0};
    uint8_t storage[4][PAGE_BYTES];
    uint8_t before[4][PAGE_BYTES];
    uint8_t *page[4] = {storage[0], storage[1], storage[2], storage[3]};
    pr_slot_a_printer_page_report report;
    pr_font201 font;
    size_t scanline, plane;

    for (scanline = 0u; scanline < 8u; ++scanline) {
        font_bytes[8u + scanline] = (uint8_t)(0x80u >> scanline);
    }
    CHECK(pr_font201_open(font_bytes, sizeof font_bytes, &font) == PR_FONT201_OK);
    memset(storage, 0xEE, sizeof storage);
    CHECK(pr_slot_a_printer_page_apply(page, PAGE_BASE, PAGE_BYTES, &font,
                                       COLUMN, ROW, 10u, "a", 1u, &report) ==
          PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == 1u && report.plane_stores == 32u &&
          report.changed_plane_bytes == 32u);
    for (scanline = 0u; scanline < 8u; ++scanline) {
        size_t at = ((ROW * 8u + scanline) * 40u + COLUMN) - PAGE_BASE;
        size_t prior_row = (((ROW - 1u) * 8u + scanline) * 40u + COLUMN) - PAGE_BASE;
        uint8_t bits = font_bytes[8u + scanline];
        for (plane = 0u; plane < 4u; ++plane) {
            CHECK(storage[plane][at] == ((plane == 1u || plane == 3u) ? bits : 0u));
            CHECK(storage[plane][at - 1u] == 0xEEu);
            CHECK(storage[plane][at + 1u] == 0xEEu);
            CHECK(storage[plane][prior_row] == 0xEEu);
        }
    }
    CHECK(pr_slot_a_printer_page_apply(page, PAGE_BASE, PAGE_BYTES, &font,
                                       COLUMN, ROW, 10u, "a", 1u, &report) ==
          PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == 1u && report.plane_stores == 32u &&
          report.changed_plane_bytes == 0u);
    memcpy(before, storage, sizeof before);
    CHECK(pr_slot_a_printer_page_apply(page, PAGE_BASE, PAGE_BYTES, &font,
                                       40u, ROW, 10u, "a", 1u, &report) ==
          PR_SLOT_A_PRINTER_PAGE_BOUNDS);
    CHECK(memcmp(before, storage, sizeof before) == 0);
    puts("synthetic slot-A row-five one-cell printer projection passed");
    return 0;
}
