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

enum { PAGE_BASE = 0x3C0, PAGE_BYTES = 3520, ROW = 9 };

static int check_page(uint8_t page[4][PAGE_BYTES], unsigned calls) {
    size_t plane, at;
    for (plane = 0u; plane < 4u; ++plane) {
        for (at = 0u; at < PAGE_BYTES; ++at) {
            size_t video = PAGE_BASE + at;
            size_t scanline = video / 40u;
            size_t column = video % 40u;
            uint8_t expected = 0xA5u;
            if (scanline >= ROW * 8u && scanline < (ROW + 1u) * 8u) {
                size_t glyph_row = scanline - ROW * 8u;
                size_t glyph = 0u;
                unsigned color = 0u;
                if (calls >= 1u && column >= 17u && column < 24u) {
                    glyph = column - 17u + 1u;
                    color = 11u;
                } else if (calls >= 2u && column == 34u) {
                    glyph = 8u;
                    color = 10u;
                } else if (calls >= 3u && column == 38u) {
                    glyph = 9u;
                    color = 10u;
                }
                if (glyph != 0u) {
                    expected = (color & (1u << plane)) != 0u ?
                        (uint8_t)(1u << ((glyph + glyph_row) & 7u)) : 0u;
                }
            }
            if (page[plane][at] != expected) {
                fprintf(stderr, "page differs at call %u plane %zu offset %zu\n",
                        calls, plane, at);
                return 0;
            }
        }
    }
    return 1;
}

int main(void) {
    uint8_t font_bytes[PR_FONT201_BLOCK_SIZE] = {0};
    uint8_t storage[4][PAGE_BYTES];
    uint8_t *page[4] = {storage[0], storage[1], storage[2], storage[3]};
    pr_slot_a_printer_page_report report;
    pr_font201 font;
    size_t glyph, scanline;

    for (glyph = 1u; glyph <= 9u; ++glyph) {
        for (scanline = 0u; scanline < 8u; ++scanline) {
            font_bytes[glyph * 8u + scanline] =
                (uint8_t)(1u << ((glyph + scanline) & 7u));
        }
    }
    CHECK(pr_font201_open(font_bytes, sizeof font_bytes, &font) == PR_FONT201_OK);
    memset(storage, 0xA5, sizeof storage);
    CHECK(check_page(storage, 0u));

    CHECK(pr_slot_a_printer_page_apply(page, PAGE_BASE, PAGE_BYTES, &font,
                                       17u, ROW, 11u, "abcdefg", 7u,
                                       &report) == PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == 7u && report.plane_stores == 224u &&
          report.changed_plane_bytes == 224u);
    CHECK(check_page(storage, 1u));

    CHECK(pr_slot_a_printer_page_apply(page, PAGE_BASE, PAGE_BYTES, &font,
                                       34u, ROW, 10u, "h", 1u,
                                       &report) == PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == 1u && report.plane_stores == 32u &&
          report.changed_plane_bytes == 32u);
    CHECK(check_page(storage, 2u));

    CHECK(pr_slot_a_printer_page_apply(page, PAGE_BASE, PAGE_BYTES, &font,
                                       38u, ROW, 10u, "i", 1u,
                                       &report) == PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == 1u && report.plane_stores == 32u &&
          report.changed_plane_bytes == 32u);
    CHECK(check_page(storage, 3u));
    puts("synthetic slot-A row-nine three-call composition passed");
    return 0;
}
