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

enum { PAGE_BASE = 0x3C0, PAGE_BYTES = 3520, ROW = 9, COLUMN = 38 };

int main(void) {
    uint8_t font_bytes[PR_FONT201_BLOCK_SIZE] = {0};
    uint8_t storage[4][PAGE_BYTES];
    uint8_t *page[4] = {storage[0], storage[1], storage[2], storage[3]};
    pr_slot_a_printer_page_report report;
    pr_font201 font;
    size_t scanline, plane;

    for (scanline = 0u; scanline < 8u; ++scanline) {
        font_bytes[8u + scanline] = (uint8_t)(0x80u >> scanline);
    }
    CHECK(pr_font201_open(font_bytes, sizeof font_bytes, &font) == PR_FONT201_OK);
    memset(storage, 0xA5, sizeof storage);
    CHECK(pr_slot_a_printer_page_apply(page, PAGE_BASE, PAGE_BYTES, &font,
                                       COLUMN, ROW, 10u, "a", 1u, &report) ==
          PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == 1u && report.plane_stores == 32u &&
          report.changed_plane_bytes == 32u);
    for (scanline = 0u; scanline < 8u; ++scanline) {
        size_t at = ((ROW * 8u + scanline) * 40u + COLUMN) - PAGE_BASE;
        size_t above = (((ROW - 1u) * 8u + scanline) * 40u + COLUMN) - PAGE_BASE;
        size_t below = (((ROW + 1u) * 8u + scanline) * 40u + COLUMN) - PAGE_BASE;
        for (plane = 0u; plane < 4u; ++plane) {
            uint8_t glyph = font_bytes[8u + scanline];
            CHECK(storage[plane][at] ==
                  ((plane == 1u || plane == 3u) ? glyph : 0u));
            CHECK(storage[plane][at - 1u] == 0xA5u);
            CHECK(storage[plane][at + 1u] == 0xA5u);
            CHECK(storage[plane][above] == 0xA5u);
            CHECK(storage[plane][below] == 0xA5u);
        }
    }
    puts("synthetic slot-A row-nine column-38 projection passed");
    return 0;
}
