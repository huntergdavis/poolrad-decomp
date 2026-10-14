#include "poolrad/recovered/slot_a_u3b0_prompt_row.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

static uint8_t font_block[PR_FONT201_BLOCK_SIZE];
static uint8_t storage[4][8000];

static int test_colors_and_clear(void) {
    pr_font201 font;
    pr_slot_a_printer_page_report report;
    uint8_t set[32] = {0};
    uint8_t *page[4] = {storage[0], storage[1], storage[2], storage[3]};
    const uint8_t chars[] = {'a', 'B', 'c'};
    pr_slot_a_u3b0_row_input input = {
        0u, 3u, chars, set, 1u, 1u, 5u, 2u
    };
    size_t first = 24u * 8u * 40u;
    size_t p;
    memset(font_block, 0, sizeof font_block);
    font_block[1u * 8u] = 0xF0u;
    font_block[2u * 8u] = 0xCCu;
    font_block[3u * 8u] = 0xAAu;
    set['B' >> 3u] = (uint8_t)(1u << ('B' & 7u));
    for (p = 0u; p < 4u; ++p) {
        memset(storage[p], 0xFF, sizeof storage[p]);
    }
    CHECK(pr_font201_open(font_block, sizeof font_block, &font) == PR_FONT201_OK);
    CHECK(pr_slot_a_u3b0_prompt_row_apply(page, &font, &input, &report) ==
          PR_SLOT_A_U3B0_ROW_OK);
    CHECK(report.cells == 39u && report.plane_stores == 1248u &&
          report.changed_plane_bytes == 1248u);
    CHECK(storage[0][first] == 0xF0u && storage[2][first] == 0xF0u);
    CHECK(storage[1][first] == 0u && storage[3][first] == 0u);
    CHECK(storage[0][first + 1u] == 0xCCu && storage[2][first + 1u] == 0xCCu);
    CHECK(storage[1][first + 2u] == 0xAAu);
    CHECK(storage[0][first + 2u] == 0u && storage[2][first + 2u] == 0u);
    CHECK(storage[0][first + 3u] == 0u && storage[1][first + 38u] == 0u);
    CHECK(storage[0][first + 39u] == 0xFFu && storage[0][first - 1u] == 0xFFu);
    CHECK(pr_slot_a_u3b0_prompt_row_apply(page, &font, &input, &report) ==
          PR_SLOT_A_U3B0_ROW_OK);
    CHECK(report.cells == 39u && report.changed_plane_bytes == 0u);
    return 0;
}

static int test_boundary_paths(void) {
    pr_font201 font;
    pr_slot_a_printer_page_report report;
    uint8_t set[32] = {0};
    uint8_t *page[4] = {storage[0], storage[1], storage[2], storage[3]};
    const uint8_t chars[] = {'A', 'B'};
    pr_slot_a_u3b0_row_input input = {
        38u, 2u, chars, set, 0u, 0u, 15u, 10u
    };
    uint8_t before[sizeof storage];
    CHECK(pr_font201_open(font_block, sizeof font_block, &font) == PR_FONT201_OK);
    memcpy(before, storage, sizeof before);
    CHECK(pr_slot_a_u3b0_prompt_row_apply(page, &font, &input, &report) ==
          PR_SLOT_A_U3B0_ROW_BOUNDS);
    CHECK(report.cells == 0u && memcmp(before, storage, sizeof before) == 0);
    input.length = 0u;
    input.characters = NULL;
    CHECK(pr_slot_a_u3b0_prompt_row_apply(page, &font, &input, &report) ==
          PR_SLOT_A_U3B0_ROW_OK);
    CHECK(report.cells == 0u && memcmp(before, storage, sizeof before) == 0);
    input.length = 2u;
    CHECK(pr_slot_a_u3b0_prompt_row_apply(page, &font, &input, &report) ==
          PR_SLOT_A_U3B0_ROW_ARGUMENT);
    CHECK(report.cells == 0u && memcmp(before, storage, sizeof before) == 0);
    return 0;
}

int main(void) {
    if (test_colors_and_clear() != 0 || test_boundary_paths() != 0) {
        return 1;
    }
    puts("slot-A unit 3B0 prompt row: synthetic page paths passed");
    return 0;
}
