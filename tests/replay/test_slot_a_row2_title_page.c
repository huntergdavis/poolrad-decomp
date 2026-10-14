#include "poolrad/recovered/slot_a_printer_page.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

enum { PAGE_BYTES = 8192, ROW = 2, CELLS = 4 };

static uint8_t font_bytes[PR_FONT201_BLOCK_SIZE];
static uint8_t storage[4][PAGE_BYTES];

static int check_title(uint8_t column, uint8_t fill) {
    uint8_t *page[4] = {storage[0], storage[1], storage[2], storage[3]};
    uint8_t before[4][PAGE_BYTES];
    pr_slot_a_printer_page_report report;
    pr_font201 font;
    size_t plane, cell, scanline;

    memset(storage, fill, sizeof storage);
    CHECK(pr_font201_open(font_bytes, sizeof font_bytes, &font) == PR_FONT201_OK);
    CHECK(pr_slot_a_printer_page_apply(page, 0u, PAGE_BYTES, &font,
                                       column, ROW, 15u, "ABCD", CELLS,
                                       &report) == PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == CELLS && report.plane_stores == 128u &&
          report.changed_plane_bytes == 128u);
    for (scanline = 0u; scanline < 8u; ++scanline) {
        for (cell = 0u; cell < CELLS; ++cell) {
            size_t at = (ROW * 8u + scanline) * 40u + column + cell;
            uint8_t expected = font_bytes[(cell + 1u) * 8u + scanline];
            for (plane = 0u; plane < 4u; ++plane) {
                CHECK(storage[plane][at] == expected);
                CHECK(storage[plane][at - cell - 1u] == fill);
                CHECK(storage[plane][at + 8u * 40u] == fill);
            }
        }
    }
    memcpy(before, storage, sizeof before);
    CHECK(pr_slot_a_printer_page_apply(page, 0u, PAGE_BYTES, &font,
                                       column, ROW, 15u, "ABCD", CELLS,
                                       &report) == PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.changed_plane_bytes == 0u);
    CHECK(pr_slot_a_printer_page_apply(page, 0u, PAGE_BYTES, &font,
                                       37u, ROW, 15u, "ABCD", CELLS,
                                       &report) == PR_SLOT_A_PRINTER_PAGE_BOUNDS);
    CHECK(pr_slot_a_printer_page_apply(page, 0u, PAGE_BYTES, &font,
                                       column, 25u, 15u, "ABCD", CELLS,
                                       &report) == PR_SLOT_A_PRINTER_PAGE_BOUNDS);
    CHECK(memcmp(before, storage, sizeof before) == 0);
    return 0;
}

int main(void) {
    static const uint8_t patterns[CELLS] = {0xA5u, 0x5Au, 0xC3u, 0x3Cu};
    size_t cell, scanline;
    for (cell = 0u; cell < CELLS; ++cell) {
        for (scanline = 0u; scanline < 8u; ++scanline) {
            font_bytes[(cell + 1u) * 8u + scanline] =
                (uint8_t)(patterns[cell] ^ (uint8_t)scanline);
        }
    }
    CHECK(check_title(17u, 0xEEu) == 0);
    CHECK(check_title(1u, 0u) == 0);
    puts("synthetic slot-A row-two title page projection passed");
    return 0;
}
