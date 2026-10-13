#include "poolrad/recovered/slot_a_printer_page.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

enum { BASE = 0x3C0, PAGE_BYTES = 3520 };

static uint8_t block[PR_FONT201_BLOCK_SIZE];
static uint8_t storage[4][PAGE_BYTES];

static int test_reached_cells(void) {
    pr_font201 font;
    pr_slot_a_printer_page_report report;
    uint8_t *page[4] = {storage[0], storage[1], storage[2], storage[3]};
    size_t at = (32u * 40u + 17u) - BASE, next = at + 1u;
    unsigned plane;

    memset(block, 0, sizeof block);
    block[1u * 8u] = 0xA5u;  /* upcase 'A' -> glyph 1 */
    block[1u * 8u + 1u] = 0x5Au;
    block[2u * 8u] = 0xC3u;  /* upcase 'B' -> glyph 2 */
    for (plane = 0u; plane < 4u; ++plane) {
        memset(storage[plane], 0xEE, PAGE_BYTES);
    }
    CHECK(pr_font201_open(block, sizeof block, &font) == PR_FONT201_OK);
    CHECK(pr_slot_a_printer_page_apply(page, BASE, PAGE_BYTES, &font,
                                       17u, 4u, 5u, "aB", 2u, &report) ==
          PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == 2u && report.plane_stores == 64u &&
          report.changed_plane_bytes == 64u);
    CHECK(storage[0][at] == 0xA5u && storage[2][at] == 0xA5u);
    CHECK(storage[0][next] == 0xC3u && storage[2][next] == 0xC3u);
    CHECK(storage[1][at] == 0u && storage[3][next] == 0u);
    CHECK(storage[0][at + 40u] == 0x5Au && storage[2][at + 40u] == 0x5Au);
    CHECK(storage[0][at - 1u] == 0xEEu && storage[0][next + 1u] == 0xEEu);
    CHECK(storage[0][at + 8u * 40u] == 0xEEu);
    CHECK(pr_slot_a_printer_page_apply(page, BASE, PAGE_BYTES, &font,
                                       17u, 4u, 5u, "aB", 2u, &report) ==
          PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == 2u && report.plane_stores == 64u &&
          report.changed_plane_bytes == 0u);
    return 0;
}

static int test_negative_routes(void) {
    pr_font201 font;
    pr_slot_a_printer_page_report report;
    uint8_t *page[4] = {storage[0], storage[1], storage[2], storage[3]};
    uint8_t before[4][PAGE_BYTES];
    CHECK(pr_font201_open(block, sizeof block, &font) == PR_FONT201_OK);
    memcpy(before, storage, sizeof before);
    CHECK(pr_slot_a_printer_page_apply(page, BASE, PAGE_BYTES, &font,
                                       17u, 4u, 15u, NULL, 0u, &report) ==
          PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == 0u && report.plane_stores == 0u &&
          report.changed_plane_bytes == 0u);
    CHECK(pr_slot_a_printer_page_apply(page, BASE, PAGE_BYTES, &font,
                                       40u, 4u, 15u, "A", 1u, &report) ==
          PR_SLOT_A_PRINTER_PAGE_BOUNDS);
    CHECK(pr_slot_a_printer_page_apply(page, BASE, PAGE_BYTES, &font,
                                       17u, 25u, 15u, "A", 1u, &report) ==
          PR_SLOT_A_PRINTER_PAGE_BOUNDS);
    CHECK(pr_slot_a_printer_page_apply(page, BASE, PAGE_BYTES, &font,
                                       39u, 4u, 15u, "AB", 2u, &report) ==
          PR_SLOT_A_PRINTER_PAGE_BOUNDS);
    CHECK(pr_slot_a_printer_page_apply(page, BASE, 100u, &font,
                                       17u, 4u, 15u, "A", 1u, &report) ==
          PR_SLOT_A_PRINTER_PAGE_BOUNDS);
    CHECK(pr_slot_a_printer_page_apply(page, BASE, PAGE_BYTES, &font,
                                       17u, 4u, 15u, NULL, 1u, &report) ==
          PR_SLOT_A_PRINTER_PAGE_ARGUMENT);
    page[2] = NULL;
    CHECK(pr_slot_a_printer_page_apply(page, BASE, PAGE_BYTES, &font,
                                       17u, 4u, 15u, "A", 1u, &report) ==
          PR_SLOT_A_PRINTER_PAGE_ARGUMENT);
    page[2] = storage[2];
    font.size = PR_FONT201_BLOCK_SIZE - 1u;
    CHECK(pr_slot_a_printer_page_apply(page, BASE, PAGE_BYTES, &font,
                                       17u, 4u, 15u, "A", 1u, &report) ==
          PR_SLOT_A_PRINTER_PAGE_ARGUMENT);
    CHECK(memcmp(before, storage, sizeof before) == 0);
    CHECK(report.cells == 0u && report.plane_stores == 0u &&
          report.changed_plane_bytes == 0u);
    return 0;
}

int main(void) {
    CHECK(test_reached_cells() == 0);
    CHECK(test_negative_routes() == 0);
    return 0;
}
