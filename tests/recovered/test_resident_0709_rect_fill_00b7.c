#include "poolrad/recovered/resident_0709_rect_fill_00b7.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

enum { PLANE_BYTES = 0x3F40, PAGE_BYTES = 0x2000 };

static uint8_t storage[4][PLANE_BYTES];
static uint8_t snapshot[4][PLANE_BYTES];

static void reset_state(pr_res0709_rect_state *state) {
    size_t lane;
    memset(state, 0xA5, sizeof *state);
    for (lane = 0; lane < 4; ++lane) {
        state->plane[lane] = storage[lane];
    }
    state->plane_bytes = PLANE_BYTES;
    state->active_page = 0;
    state->draw_segment = 0xA400u;
    memset(storage, 0xA5, sizeof storage);
}

static int test_reached_two_page_clear(void) {
    pr_res0709_rect_state state;
    pr_res0709_rect_report report;
    pr_res0709_rect_args args = {0u, 1u, 22u, 38u, 17u, 1u};
    size_t plane, at;

    reset_state(&state);
    CHECK(pr_res0709_rect_fill_00b7(&state, args, &report) == PR_RES0709_RECT_OK);
    CHECK(report.selected_segment == 0xA200u);
    CHECK(report.restored_segment == 0xA000u);
    CHECK(report.plane_byte_stores == 4u * 38u * 48u);
    CHECK(report.changed_plane_bytes == report.plane_byte_stores);
    CHECK(state.active_page == 0u && state.draw_segment == 0xA000u);
    CHECK(state.gc_index == 5u && state.gc[5] == 0u);
    CHECK(state.gc[6] == 0xA5u && state.gc[7] == 0x0Fu &&
          state.gc[8] == 0xFFu);
    CHECK(state.seq_index == 2u && state.seq_map_mask == 0x0Fu);
    for (plane = 0; plane <= 4; ++plane) {
        CHECK(state.gc[plane] == 0u);
    }
    for (plane = 0; plane < 4; ++plane) {
        for (at = 0; at < PLANE_BYTES; ++at) {
            size_t relative = at >= PAGE_BYTES ? at - PAGE_BYTES : PLANE_BYTES;
            size_t scanline = relative / 40u;
            size_t column = relative % 40u;
            uint8_t expected = at >= PAGE_BYTES &&
                               scanline >= 17u * 8u && scanline < 23u * 8u &&
                               column >= 1u && column <= 38u ? 0u : 0xA5u;
            CHECK(storage[plane][at] == expected);
        }
    }
    CHECK(pr_res0709_rect_fill_00b7(&state, args, &report) == PR_RES0709_RECT_OK);
    CHECK(report.plane_byte_stores == 4u * 38u * 48u);
    CHECK(report.changed_plane_bytes == 0u);
    return 0;
}

static int test_mode_two_color_and_restore(void) {
    pr_res0709_rect_state state;
    pr_res0709_rect_report report;
    pr_res0709_rect_args args = {0xF5u, 0u, 3u, 4u, 2u, 3u};
    size_t plane, at;

    reset_state(&state);
    state.active_page = 1u;
    CHECK(pr_res0709_rect_fill_00b7(&state, args, &report) == PR_RES0709_RECT_OK);
    CHECK(report.selected_segment == 0xA000u);
    CHECK(report.restored_segment == 0xA200u);
    CHECK(state.draw_segment == 0xA200u && state.active_page == 1u);
    CHECK(report.plane_byte_stores == 4u * 2u * 16u);
    CHECK(report.changed_plane_bytes == report.plane_byte_stores);
    for (plane = 0; plane < 4; ++plane) {
        for (at = 0; at < PLANE_BYTES; ++at) {
            size_t line = at / 40u;
            size_t column = at % 40u;
            uint8_t expected = line >= 16u && line < 32u &&
                               column >= 3u && column <= 4u ?
                               ((plane == 0u || plane == 2u) ? 0xFFu : 0u) :
                               0xA5u;
            CHECK(storage[plane][at] == expected);
        }
    }
    return 0;
}

static int test_boundary_has_no_effect(void) {
    pr_res0709_rect_state state, before;
    pr_res0709_rect_report report;
    pr_res0709_rect_args args = {0u, 1u, 24u, 39u, 24u, 0u};

    reset_state(&state);
    before = state;
    memcpy(snapshot, storage, sizeof snapshot);
    state.plane_bytes = PLANE_BYTES - 1u;
    before.plane_bytes = PLANE_BYTES - 1u;
    CHECK(pr_res0709_rect_fill_00b7(&state, args, &report) == PR_RES0709_RECT_BOUNDARY);
    CHECK(report.plane_byte_stores == 0u);
    CHECK(memcmp(&state, &before, sizeof state) == 0);
    CHECK(memcmp(storage, snapshot, sizeof storage) == 0);

    state.plane_bytes = PLANE_BYTES;
    args.right = 40u;
    CHECK(pr_res0709_rect_fill_00b7(&state, args, &report) == PR_RES0709_RECT_BOUNDARY);
    args.right = 39u;
    args.left = 0xFFu;
    CHECK(pr_res0709_rect_fill_00b7(&state, args, &report) == PR_RES0709_RECT_BOUNDARY);
    args.left = 0u;
    args.page = 8u;
    CHECK(pr_res0709_rect_fill_00b7(&state, args, &report) == PR_RES0709_RECT_BOUNDARY);
    CHECK(memcmp(storage, snapshot, sizeof storage) == 0);
    CHECK(pr_res0709_rect_fill_00b7(NULL, args, &report) == PR_RES0709_RECT_ARGUMENT);
    CHECK(pr_res0709_rect_fill_00b7(&state, args, NULL) == PR_RES0709_RECT_ARGUMENT);
    return 0;
}

int main(void) {
    if (test_reached_two_page_clear()) return 1;
    if (test_mode_two_color_and_restore()) return 1;
    if (test_boundary_has_no_effect()) return 1;
    puts("synthetic RES_0709:00B7 bounded rectangle passed");
    return 0;
}
