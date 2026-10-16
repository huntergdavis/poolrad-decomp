#include "poolrad/replay/cold_start_credits_expected_cells.h"

#include <stdio.h>
#include <string.h>

#define OVR_SIZE 0x06E9u
#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static uint8_t ovr[OVR_SIZE];
static pr_cold_credits_expected_cell cells[PR_COLD_CREDITS_EXPECTED_CELL_COUNT];
static pr_cold_credits_expected_cell sentinel[PR_COLD_CREDITS_EXPECTED_CELL_COUNT];
static size_t source_offsets[29u];

static void make_fixture(void) {
    static const uint8_t layout[29u] = {
        0xB0u, 0u, 0x50u, 0xB0u, 0u, 0x50u, 0xB0u, 0u, 0x50u,
        0x8Du, 0x7Eu, 0u, 0x16u, 0x57u, 0xBFu, 0u, 0u, 0x0Eu,
        0x57u, 0x9Au, 0x0Bu, 0x0Bu, 0xF8u, 0x0Au,
        0x9Au, 0x26u, 0x06u, 0x09u, 0x07u
    };
    size_t source = 0x011Fu, i, j;
    memset(ovr, 0, sizeof ovr);
    for (i = 0u; i < 29u; ++i) {
        size_t length;
        if (i == 0u) length = 12u;
        else if (i == 1u || i == 2u) length = 10u;
        else if (i == 3u) length = 13u;
        else if (i < 10u) length = 14u;
        else length = 15u;
        source_offsets[i] = source - 0x00D4u;
        ovr[source++] = (uint8_t)length;
        for (j = 0u; j < length; ++j)
            ovr[source++] = (uint8_t)('A' + (int)(i % 26u));
    }
    for (i = 0u; i < 33u; ++i) {
        size_t source_index = i < 29u ? i : i - 29u;
        size_t source_file = source_offsets[source_index] + 0x00D4u;
        size_t length = ovr[source_file];
        uint8_t *r = ovr + 0x032Cu + i * 29u;
        memcpy(r, layout, sizeof layout);
        r[1] = (uint8_t)((i % 2u) ? 21u : 1u);
        r[4] = (uint8_t)(1u + i / 2u);
        r[7] = (uint8_t)((i % 3u) == 0u ? 10u :
                         (i % 3u) == 1u ? 11u : 14u);
        r[11] = (uint8_t)(0u - (length + 1u));
        r[15] = (uint8_t)source_offsets[source_index];
        r[16] = (uint8_t)(source_offsets[source_index] >> 8u);
    }
}

static int test_map(void) {
    size_t needed = 0u, i;
    uint8_t seen[40u * 25u] = {0};
    make_fixture();
    CHECK(pr_cold_credits_expected_cells(ovr, sizeof ovr,
          cells, PR_COLD_CREDITS_EXPECTED_CELL_COUNT, &needed) ==
          PR_COLD_CREDITS_CELLS_OK);
    CHECK(needed == PR_COLD_CREDITS_EXPECTED_CELL_COUNT);
    CHECK(cells[0].row == 1u && cells[0].column == 1u &&
          cells[0].color == 10u && cells[0].text_byte == 'A' &&
          cells[0].call_index == 0u && cells[0].text_index == 0u);
    CHECK(cells[12].row == 1u && cells[12].column == 21u &&
          cells[12].color == 11u && cells[12].text_byte == 'B' &&
          cells[12].call_index == 1u);
    CHECK(cells[446].row == 17u && cells[446].column == 1u &&
          cells[446].color == 14u && cells[446].text_byte == 'D' &&
          cells[446].call_index == 32u && cells[446].text_index == 0u);
    CHECK(cells[458].row == 17u && cells[458].column == 13u &&
          cells[458].text_index == 12u);
    for (i = 0u; i < needed; ++i) {
        size_t index = (size_t)cells[i].row * 40u + cells[i].column;
        CHECK(index < sizeof seen && !seen[index]);
        seen[index] = 1u;
    }
    return 0;
}

static int test_rejections(void) {
    size_t needed = 123u;
    make_fixture();
    memset(cells, 0xA5, sizeof cells);
    memcpy(sentinel, cells, sizeof cells);
    CHECK(pr_cold_credits_expected_cells(ovr, sizeof ovr, cells,
          PR_COLD_CREDITS_EXPECTED_CELL_COUNT - 1u, &needed) ==
          PR_COLD_CREDITS_CELLS_OUTPUT_SIZE);
    CHECK(needed == PR_COLD_CREDITS_EXPECTED_CELL_COUNT);
    CHECK(memcmp(cells, sentinel, sizeof cells) == 0);
    CHECK(pr_cold_credits_expected_cells(ovr, sizeof ovr, NULL, 0u,
          &needed) == PR_COLD_CREDITS_CELLS_OUTPUT_SIZE);
    CHECK(needed == PR_COLD_CREDITS_EXPECTED_CELL_COUNT);
    CHECK(pr_cold_credits_expected_cells(NULL, sizeof ovr, cells,
          PR_COLD_CREDITS_EXPECTED_CELL_COUNT, &needed) ==
          PR_COLD_CREDITS_CELLS_ARGUMENT && needed == 0u);
    CHECK(pr_cold_credits_expected_cells(ovr, sizeof ovr, cells,
          PR_COLD_CREDITS_EXPECTED_CELL_COUNT, NULL) ==
          PR_COLD_CREDITS_CELLS_ARGUMENT);
    CHECK(pr_cold_credits_expected_cells(ovr, sizeof ovr - 1u, cells,
          PR_COLD_CREDITS_EXPECTED_CELL_COUNT, &needed) ==
          PR_COLD_CREDITS_CELLS_INPUT_SIZE && needed == 0u);

    make_fixture();
    ovr[0x011Fu] = 0u;
    CHECK(pr_cold_credits_expected_cells(ovr, sizeof ovr, cells,
          PR_COLD_CREDITS_EXPECTED_CELL_COUNT, &needed) ==
          PR_COLD_CREDITS_CELLS_SOURCE && needed == 0u);
    make_fixture();
    ovr[0x032Cu + 19u] = 0u;
    CHECK(pr_cold_credits_expected_cells(ovr, sizeof ovr, cells,
          PR_COLD_CREDITS_EXPECTED_CELL_COUNT, &needed) ==
          PR_COLD_CREDITS_CELLS_CALL && needed == 0u);
    make_fixture();
    ovr[0x032Cu + 11u] = 0u;
    CHECK(pr_cold_credits_expected_cells(ovr, sizeof ovr, cells,
          PR_COLD_CREDITS_EXPECTED_CELL_COUNT, &needed) ==
          PR_COLD_CREDITS_CELLS_CALL && needed == 0u);
    make_fixture();
    ovr[0x032Cu + 15u] = 0u;
    CHECK(pr_cold_credits_expected_cells(ovr, sizeof ovr, cells,
          PR_COLD_CREDITS_EXPECTED_CELL_COUNT, &needed) ==
          PR_COLD_CREDITS_CELLS_SOURCE && needed == 0u);
    make_fixture();
    ovr[0x032Cu + 29u + 1u] = 1u;
    CHECK(pr_cold_credits_expected_cells(ovr, sizeof ovr, cells,
          PR_COLD_CREDITS_EXPECTED_CELL_COUNT, &needed) ==
          PR_COLD_CREDITS_CELLS_PLACEMENT && needed == 0u);
    CHECK(memcmp(cells, sentinel, sizeof cells) == 0);
    return 0;
}

int main(void) {
    if (test_map() || test_rejections()) return 1;
    puts("synthetic cold-start credits expected-cell tests passed");
    return 0;
}
