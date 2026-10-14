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

enum { PAGE_BASE = 0, PAGE_BYTES = 8000, COLUMNS = 40 };

static uint8_t glyph_byte(size_t glyph, size_t row) {
    return (uint8_t)(1u << ((glyph + row) & 7u));
}

static void seed_row_two(uint8_t page[4][PAGE_BYTES]) {
    size_t lane, row, glyph;
    for (lane = 0u; lane < 4u; ++lane) {
        for (row = 0u; row < 8u; ++row) {
            for (glyph = 13u; glyph <= 16u; ++glyph) {
                page[lane][(2u * 8u + row) * COLUMNS + 17u + glyph - 13u] =
                    glyph_byte(glyph, row);
            }
            for (glyph = 17u; glyph <= 22u; ++glyph) {
                page[lane][(2u * 8u + row) * COLUMNS + 33u + glyph - 17u] =
                    glyph_byte(glyph, row);
            }
        }
    }
}

int main(void) {
    uint8_t font_bytes[PR_FONT201_BLOCK_SIZE] = {0};
    uint8_t storage[4][PAGE_BYTES];
    uint8_t expected[4][PAGE_BYTES];
    uint8_t *page[4] = {storage[0], storage[1], storage[2], storage[3]};
    pr_slot_a_printer_page_report report;
    pr_font201 font;
    size_t glyph, row, lane;

    for (glyph = 1u; glyph <= 22u; ++glyph) {
        for (row = 0u; row < 8u; ++row) {
            font_bytes[glyph * 8u + row] = glyph_byte(glyph, row);
        }
    }
    CHECK(pr_font201_open(font_bytes, sizeof font_bytes, &font) == PR_FONT201_OK);
    memset(storage, 0xA5, sizeof storage);
    memset(expected, 0xA5, sizeof expected);
    seed_row_two(storage);
    seed_row_two(expected);

    CHECK(pr_slot_a_printer_page_apply(page, PAGE_BASE, PAGE_BYTES, &font,
                                       17u, 15u, 10u, "abcdefghijkl", 12u,
                                       &report) == PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == 12u && report.plane_stores == 384u &&
          report.changed_plane_bytes == 384u);
    for (lane = 0u; lane < 4u; ++lane) {
        for (row = 0u; row < 8u; ++row) {
            for (glyph = 1u; glyph <= 12u; ++glyph) {
                expected[lane][(15u * 8u + row) * COLUMNS + 17u + glyph - 1u] =
                    (lane == 1u || lane == 3u) ? glyph_byte(glyph, row) : 0u;
            }
        }
    }
    CHECK(memcmp(storage, expected, sizeof storage) == 0);

    CHECK(pr_slot_a_printer_page_apply(page, PAGE_BASE, PAGE_BYTES, &font,
                                       17u, 2u, 15u, "mnop", 4u,
                                       &report) == PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == 4u && report.plane_stores == 128u &&
          report.changed_plane_bytes == 0u);
    CHECK(memcmp(storage, expected, sizeof storage) == 0);

    CHECK(pr_slot_a_printer_page_apply(page, PAGE_BASE, PAGE_BYTES, &font,
                                       33u, 2u, 15u, "qrstuv", 6u,
                                       &report) == PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == 6u && report.plane_stores == 192u &&
          report.changed_plane_bytes == 0u);
    CHECK(memcmp(storage, expected, sizeof storage) == 0);
    puts("synthetic slot-A post-row-nine full-page composition passed");
    return 0;
}
