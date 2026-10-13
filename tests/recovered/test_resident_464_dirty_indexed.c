#include "poolrad/formats/ega.h"
#include "poolrad/recovered/resident_464_dirty_transfer.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

static int pixel_byte(const pr_r464_dirty_state *state, size_t at,
                      const uint8_t expected[8]) {
    uint8_t planes[4], pixels[8];
    unsigned lane;
    for (lane = 0; lane < 4; ++lane) planes[lane] = state->page[lane][at];
    CHECK(pr_ega_planar_byte(planes, pixels) == PR_EGA_OK);
    CHECK(memcmp(pixels, expected, 8) == 0);
    return 0;
}

int main(void) {
    pr_r464_dirty_state state = {0};
    pr_r464_dirty_report report = {0};
    uint8_t stage[4][80] = {{0}}, page[4][80] = {{0}};
    uint8_t object[128] = {0};
    const uint8_t left_clean[8] = {1, 0, 0, 0, 0, 0, 0, 8};
    const uint8_t first_dirty[8] = {1, 2, 4, 8, 0, 0, 0, 0};
    const uint8_t right_clean[8] = {4, 0, 0, 0, 0, 0, 0, 0};
    const uint8_t second_dirty[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    unsigned lane;

    state.vram_size = 80;
    state.gc_bit_mask = 0xFF;
    for (lane = 0; lane < 4; ++lane) {
        state.staging[lane] = stage[lane];
        state.page[lane] = page[lane];
        state.map_mask[lane] = (uint8_t)(1u << lane);
    }
    state.row[0].dirty = state.row[1].dirty = 1;
    state.row[0].first = 0x40;
    state.row[0].last = 0x43;
    state.row[1].first = 0x44;
    state.row[1].last = 0x47;
    state.row[1].stage_base = 40;
    page[0][0] = 0x80;
    page[3][0] = 0x01;
    page[2][2] = 0x80;
    object[0x17 + 0x40] = 0x80;
    object[0x17 + 0x41] = 0x40;
    object[0x17 + 0x42] = 0x20;
    object[0x17 + 0x43] = 0x10;
    object[0x17 + 0x44] = 0xFF;

    CHECK(pr_r464_dirty_transfer(&state, object, sizeof object,
                                 &report) == PR_R464_DIRTY_OK);
    CHECK(report.dirty_rows == 2 && report.transfer_bytes == 2);
    CHECK(pixel_byte(&state, 0, left_clean) == 0);
    CHECK(pixel_byte(&state, 1, first_dirty) == 0);
    CHECK(pixel_byte(&state, 2, right_clean) == 0);
    CHECK(pixel_byte(&state, 41, second_dirty) == 0);
    CHECK(page[0][0] == 0x80 && page[3][0] == 0x01 && page[2][2] == 0x80);
    return 0;
}
