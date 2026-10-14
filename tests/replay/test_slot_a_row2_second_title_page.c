#include "poolrad/recovered/slot_a_printer_page.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

enum { PAGE_BYTES = 8192, ROW = 2, COLUMN = 33, CELLS = 6 };

static uint8_t font_bytes[PR_FONT201_BLOCK_SIZE];
static uint8_t storage[4][PAGE_BYTES];
static uint8_t snapshot[4][PAGE_BYTES];

int main(void) {
    static const uint8_t patterns[CELLS] = {
        0xA5u, 0x5Au, 0xC3u, 0x3Cu, 0xF0u, 0x0Fu
    };
    uint8_t *page[4] = {storage[0], storage[1], storage[2], storage[3]};
    pr_slot_a_printer_page_report report;
    pr_font201 font;
    size_t cell, scanline, plane, at;

    for (cell = 0u; cell < CELLS; ++cell) {
        for (scanline = 0u; scanline < 8u; ++scanline) {
            font_bytes[(cell + 1u) * 8u + scanline] =
                (uint8_t)(patterns[cell] ^ (uint8_t)scanline);
        }
    }
    memset(storage, 0xEE, sizeof storage);
    CHECK(pr_font201_open(font_bytes, sizeof font_bytes, &font) == PR_FONT201_OK);
    CHECK(pr_slot_a_printer_page_apply(page, 0u, PAGE_BYTES, &font,
                                       COLUMN, ROW, 15u, "ABCDEF", CELLS,
                                       &report) == PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == CELLS && report.plane_stores == 192u &&
          report.changed_plane_bytes == 192u);
    for (plane = 0u; plane < 4u; ++plane) {
        for (at = 0u; at < PAGE_BYTES; ++at) {
            uint8_t expected = 0xEEu;
            size_t y = at / 40u;
            size_t x = at % 40u;
            if (y >= ROW * 8u && y < (ROW + 1u) * 8u &&
                x >= COLUMN && x < COLUMN + CELLS) {
                expected = font_bytes[(x - COLUMN + 1u) * 8u +
                                      (y - ROW * 8u)];
            }
            CHECK(storage[plane][at] == expected);
        }
    }
    memcpy(snapshot, storage, sizeof snapshot);
    CHECK(pr_slot_a_printer_page_apply(page, 0u, PAGE_BYTES, &font,
                                       COLUMN, ROW, 15u, "ABCDEF", CELLS,
                                       &report) == PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.plane_stores == 192u && report.changed_plane_bytes == 0u);
    CHECK(pr_slot_a_printer_page_apply(page, 0u, PAGE_BYTES, &font,
                                       34u, ROW, 15u, "ABCDEF", CELLS,
                                       &report) == PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(memcmp(snapshot, storage, sizeof snapshot) != 0);
    memcpy(snapshot, storage, sizeof snapshot);
    CHECK(pr_slot_a_printer_page_apply(page, 0u, PAGE_BYTES, &font,
                                       35u, ROW, 15u, "ABCDEF", CELLS,
                                       &report) == PR_SLOT_A_PRINTER_PAGE_BOUNDS);
    CHECK(memcmp(snapshot, storage, sizeof snapshot) == 0);
    puts("synthetic six-cell row-two page projection passed");
    return 0;
}
