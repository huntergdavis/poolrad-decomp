#include "poolrad/recovered/slot_a_second_cycle_zero_row.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

enum { PAGE_BASE = 4 * 8 * 40, PAGE_BYTES = 6 * 8 * 40 };

static int check_page(uint8_t page[4][PAGE_BYTES], unsigned cleared_row) {
    size_t plane, at;
    for (plane = 0u; plane < 4u; ++plane) {
        for (at = 0u; at < PAGE_BYTES; ++at) {
            size_t video = PAGE_BASE + at;
            size_t row = video / (8u * 40u);
            size_t col = video % 40u;
            uint8_t expected = (row == cleared_row && col >= 17u && col <= 38u) ?
                0u : 0xA5u;
            if (page[plane][at] != expected) {
                return 0;
            }
        }
    }
    return 1;
}

int main(void) {
    uint8_t storage[4][PAGE_BYTES];
    uint8_t snapshot[4][PAGE_BYTES];
    uint8_t *page[4] = {storage[0], storage[1], storage[2], storage[3]};
    pr_slot_a_second_zero_row_report report;
    size_t plane, scanline, col;

    memset(storage, 0xA5, sizeof storage);
    CHECK(pr_slot_a_second_zero_row_apply(page, PAGE_BASE, PAGE_BYTES,
           4u, 17u, 38u, 0u, &report) == PR_SLOT_A_SECOND_ZERO_ROW_OK);
    CHECK(report.plane_stores == 704u && report.changed_plane_bytes == 704u);
    CHECK(check_page(storage, 4u));

    memset(storage, 0xA5, sizeof storage);
    CHECK(pr_slot_a_second_zero_row_apply(page, PAGE_BASE, PAGE_BYTES,
           5u, 17u, 38u, 0u, &report) == PR_SLOT_A_SECOND_ZERO_ROW_OK);
    CHECK(report.plane_stores == 704u && report.changed_plane_bytes == 704u);
    CHECK(check_page(storage, 5u));
    CHECK(pr_slot_a_second_zero_row_apply(page, PAGE_BASE, PAGE_BYTES,
           5u, 17u, 38u, 0u, &report) == PR_SLOT_A_SECOND_ZERO_ROW_OK);
    CHECK(report.plane_stores == 704u && report.changed_plane_bytes == 0u);
    CHECK(check_page(storage, 5u));

    memset(storage, 0xA5, sizeof storage);
    for (plane = 0u; plane < 4u; ++plane) {
        for (scanline = 9u * 8u; scanline < 10u * 8u; ++scanline) {
            for (col = 17u; col <= 38u; ++col) {
                storage[plane][scanline * 40u + col - PAGE_BASE] = 0u;
            }
        }
    }
    CHECK(pr_slot_a_second_zero_row_apply(page, PAGE_BASE, PAGE_BYTES,
           9u, 17u, 38u, 0u, &report) == PR_SLOT_A_SECOND_ZERO_ROW_OK);
    CHECK(report.plane_stores == 704u && report.changed_plane_bytes == 0u);
    CHECK(check_page(storage, 9u));

    memcpy(snapshot, storage, sizeof snapshot);
    CHECK(pr_slot_a_second_zero_row_apply(page, PAGE_BASE, PAGE_BYTES,
           3u, 17u, 38u, 0u, &report) == PR_SLOT_A_SECOND_ZERO_ROW_BOUNDARY);
    CHECK(pr_slot_a_second_zero_row_apply(page, PAGE_BASE, PAGE_BYTES,
           10u, 17u, 38u, 0u, &report) == PR_SLOT_A_SECOND_ZERO_ROW_BOUNDARY);
    CHECK(pr_slot_a_second_zero_row_apply(page, PAGE_BASE, PAGE_BYTES,
           5u, 18u, 38u, 0u, &report) == PR_SLOT_A_SECOND_ZERO_ROW_BOUNDARY);
    CHECK(pr_slot_a_second_zero_row_apply(page, PAGE_BASE, PAGE_BYTES,
           5u, 17u, 39u, 0u, &report) == PR_SLOT_A_SECOND_ZERO_ROW_BOUNDARY);
    CHECK(pr_slot_a_second_zero_row_apply(page, PAGE_BASE, PAGE_BYTES,
           5u, 17u, 38u, 1u, &report) == PR_SLOT_A_SECOND_ZERO_ROW_BOUNDARY);
    CHECK(pr_slot_a_second_zero_row_apply(page, PAGE_BASE + 18u, PAGE_BYTES - 18u,
           4u, 17u, 38u, 0u, &report) == PR_SLOT_A_SECOND_ZERO_ROW_BOUNDARY);
    CHECK(memcmp(storage, snapshot, sizeof storage) == 0);
    CHECK(pr_slot_a_second_zero_row_apply(NULL, PAGE_BASE, PAGE_BYTES,
           5u, 17u, 38u, 0u, &report) == PR_SLOT_A_SECOND_ZERO_ROW_ARGUMENT);
    CHECK(pr_slot_a_second_zero_row_apply(page, PAGE_BASE, PAGE_BYTES,
           5u, 17u, 38u, 0u, NULL) == PR_SLOT_A_SECOND_ZERO_ROW_ARGUMENT);
    puts("synthetic slot-A second-cycle zero row passed");
    return 0;
}
