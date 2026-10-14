#include "poolrad/recovered/slot_a_printer_page.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

enum { PAGE_BASE = 0x3C0, PAGE_BYTES = 3520, ROW = 9, COLUMN = 17, CELLS = 7 };

int main(void) {
    uint8_t font_bytes[PR_FONT201_BLOCK_SIZE] = {0};
    uint8_t storage[4][PAGE_BYTES];
    uint8_t before[4][PAGE_BYTES];
    uint8_t *page[4] = {storage[0], storage[1], storage[2], storage[3]};
    pr_slot_a_printer_page_report report;
    pr_font201 font;
    size_t cell, scanline, plane;

    /* Seven distinct invented glyphs expose wrong row or cell placement. */
    for (cell = 0u; cell < CELLS; ++cell) {
        for (scanline = 0u; scanline < 8u; ++scanline) {
            font_bytes[(cell + 1u) * 8u + scanline] =
                (uint8_t)(1u << ((cell + scanline) & 7u));
        }
    }
    CHECK(pr_font201_open(font_bytes, sizeof font_bytes, &font) == PR_FONT201_OK);
    memset(storage, 0xA5, sizeof storage);
    CHECK(pr_slot_a_printer_page_apply(page, PAGE_BASE, PAGE_BYTES, &font,
                                       COLUMN, ROW, 11u, "abcdefg", CELLS,
                                       &report) == PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == CELLS && report.plane_stores == 224u &&
          report.changed_plane_bytes == 224u);
    for (scanline = 0u; scanline < 8u; ++scanline) {
        size_t at = ((ROW * 8u + scanline) * 40u + COLUMN) - PAGE_BASE;
        size_t previous = (((ROW - 1u) * 8u + scanline) * 40u + COLUMN) - PAGE_BASE;
        for (plane = 0u; plane < 4u; ++plane) {
            for (cell = 0u; cell < CELLS; ++cell) {
                uint8_t glyph = font_bytes[(cell + 1u) * 8u + scanline];
                CHECK(storage[plane][at + cell] ==
                      ((plane == 0u || plane == 1u || plane == 3u) ? glyph : 0u));
            }
            CHECK(storage[plane][at - 1u] == 0xA5u);
            CHECK(storage[plane][at + CELLS] == 0xA5u);
            CHECK(storage[plane][previous] == 0xA5u);
        }
    }
    CHECK(pr_slot_a_printer_page_apply(page, PAGE_BASE, PAGE_BYTES, &font,
                                       COLUMN, ROW, 11u, "abcdefg", CELLS,
                                       &report) == PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == CELLS && report.plane_stores == 224u &&
          report.changed_plane_bytes == 0u);
    memcpy(before, storage, sizeof before);
    CHECK(pr_slot_a_printer_page_apply(page, PAGE_BASE, PAGE_BYTES, &font,
                                       34u, ROW, 11u, "abcdefg", CELLS,
                                       &report) == PR_SLOT_A_PRINTER_PAGE_BOUNDS);
    CHECK(memcmp(before, storage, sizeof before) == 0);
    puts("synthetic slot-A row-nine seven-cell projection passed");
    return 0;
}
