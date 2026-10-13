#include "poolrad/recovered/slot_a_printer_page.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

enum { PAGE_BASE = 0x3C0, PAGE_BYTES = 3520, COLUMN = 17, ROW = 5 };

int main(void) {
    uint8_t font_bytes[PR_FONT201_BLOCK_SIZE] = {0};
    uint8_t storage[4][PAGE_BYTES];
    uint8_t before[4][PAGE_BYTES];
    uint8_t *page[4] = {storage[0], storage[1], storage[2], storage[3]};
    pr_slot_a_printer_page_report report;
    pr_font201 font;
    size_t scanline, cell, plane;

    for (cell = 0u; cell < 6u; ++cell) {
        for (scanline = 0u; scanline < 8u; ++scanline) {
            font_bytes[(cell + 1u) * 8u + scanline] =
                (uint8_t)(1u << ((cell + scanline) % 8u));
        }
    }
    CHECK(pr_font201_open(font_bytes, sizeof font_bytes, &font) == PR_FONT201_OK);
    memset(storage, 0xEE, sizeof storage);
    CHECK(pr_slot_a_printer_page_apply(page, PAGE_BASE, PAGE_BYTES, &font,
                                       COLUMN, ROW, 11u, "abcdef", 6u, &report) ==
          PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == 6u && report.plane_stores == 192u &&
          report.changed_plane_bytes == 192u);
    for (scanline = 0u; scanline < 8u; ++scanline) {
        for (cell = 0u; cell < 6u; ++cell) {
            size_t at = ((ROW * 8u + scanline) * 40u + COLUMN + cell) - PAGE_BASE;
            uint8_t bits = font_bytes[(cell + 1u) * 8u + scanline];
            for (plane = 0u; plane < 4u; ++plane) {
                CHECK(storage[plane][at] == (plane == 2u ? 0u : bits));
            }
        }
        for (plane = 0u; plane < 4u; ++plane) {
            size_t at = ((ROW * 8u + scanline) * 40u + COLUMN) - PAGE_BASE;
            CHECK(storage[plane][at - 1u] == 0xEEu);
            CHECK(storage[plane][at + 6u] == 0xEEu);
        }
    }
    CHECK(pr_slot_a_printer_page_apply(page, PAGE_BASE, PAGE_BYTES, &font,
                                       COLUMN, ROW, 11u, "abcdef", 6u, &report) ==
          PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == 6u && report.plane_stores == 192u &&
          report.changed_plane_bytes == 0u);
    memcpy(before, storage, sizeof before);
    CHECK(pr_slot_a_printer_page_apply(page, PAGE_BASE, PAGE_BYTES, &font,
                                       35u, ROW, 11u, "abcdef", 6u, &report) ==
          PR_SLOT_A_PRINTER_PAGE_BOUNDS);
    CHECK(memcmp(before, storage, sizeof before) == 0);
    puts("synthetic slot-A six-cell printer projection passed");
    return 0;
}
