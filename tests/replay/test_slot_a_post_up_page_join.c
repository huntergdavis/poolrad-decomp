#include "poolrad/recovered/slot_a_post_up_page_join.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

enum { PLANE_BYTES = 16192, PAGE_STRIDE = 8192, ROW15_AT = 120 * 40 + 17 };

static uint8_t storage[4][PLANE_BYTES];
static uint8_t font_bytes[PR_FONT201_BLOCK_SIZE];

static void reset_storage(void) {
    memset(storage, 0xA5, sizeof storage);
    memset(font_bytes, 0, sizeof font_bytes);
    for (size_t row = 0u; row < 7u; ++row) {
        font_bytes[1u * 8u + row] = 0x3Cu;
        font_bytes[2u * 8u + row] = 0xC3u;
    }
}

static int test_reached_page_value_route(void) {
    uint8_t *plane[4] = {storage[0], storage[1], storage[2], storage[3]};
    pr_post_up_page_join route;
    pr_font201 font;
    pr_post_up_page_print_call call;
    pr_post_up_page_print_report print_report;
    pr_res0709_rect_report clear_report;
    pr_res0709_rect_args clear = {0u, 0u, 15u, 38u, 15u, 17u};
    size_t lane, row;

    reset_storage();
    CHECK(pr_font201_open(font_bytes, sizeof font_bytes, &font) == PR_FONT201_OK);
    CHECK(pr_post_up_page_join_open(&route, plane, PLANE_BYTES, 0u, 0xA000u) ==
          PR_POST_UP_PAGE_JOIN_OK);
    CHECK(pr_post_up_page_join_clear(&route, clear, &clear_report) ==
          PR_POST_UP_PAGE_JOIN_ORDER);
    call = (pr_post_up_page_print_call){
        &font, "A", 1u, 17u, 15u, 10u, {0xA000u, 0xA000u}
    };
    CHECK(pr_post_up_page_join_print(&route, &call, &print_report) ==
          PR_POST_UP_PAGE_JOIN_OK);
    CHECK(route.phase == PR_POST_UP_PAGE_ROW15_CLEAR);
    CHECK(print_report.distinct_pages == 1u);
    CHECK(print_report.page[0].cells == 1u &&
          print_report.page[0].plane_stores == 32u);
    CHECK(print_report.page[1].plane_stores == 0u);
    for (lane = 0u; lane < 4u; ++lane) {
        for (row = 0u; row < 8u; ++row) {
            size_t at = ROW15_AT + row * 40u;
            uint8_t glyph = row < 7u ? 0x3Cu : 0u;
            CHECK(storage[lane][at] == ((lane == 1u || lane == 3u) ?
                                        glyph : 0u));
            CHECK(storage[lane][PAGE_STRIDE + at] == 0xA5u);
        }
    }
    clear.right = 39u;
    CHECK(pr_post_up_page_join_clear(&route, clear, &clear_report) ==
          PR_POST_UP_PAGE_JOIN_BOUNDARY);
    CHECK(route.phase == PR_POST_UP_PAGE_ROW15_CLEAR);
    CHECK(storage[1][ROW15_AT] == 0x3Cu);
    clear.right = 38u;
    CHECK(pr_post_up_page_join_clear(&route, clear, &clear_report) ==
          PR_POST_UP_PAGE_JOIN_OK);
    CHECK(clear_report.plane_byte_stores == 704u);
    CHECK(clear_report.selected_segment == 0xA000u &&
          clear_report.restored_segment == 0xA000u);
    CHECK(route.video.active_page == 0u &&
          route.video.draw_segment == 0xA000u);
    for (lane = 0u; lane < 4u; ++lane) {
        CHECK(storage[lane][ROW15_AT] == 0u);
        CHECK(storage[lane][ROW15_AT - 1u] == 0xA5u);
        CHECK(storage[lane][PAGE_STRIDE + ROW15_AT] == 0xA5u);
    }
    call.text = "B";
    CHECK(pr_post_up_page_join_print(&route, &call, &print_report) ==
          PR_POST_UP_PAGE_JOIN_OK);
    CHECK(route.phase == PR_POST_UP_PAGE_DONE);
    CHECK(print_report.distinct_pages == 1u);
    for (lane = 0u; lane < 4u; ++lane) {
        CHECK(storage[lane][ROW15_AT] == ((lane == 1u || lane == 3u) ?
                                           0xC3u : 0u));
        CHECK(storage[lane][PAGE_STRIDE + ROW15_AT] == 0xA5u);
    }
    CHECK(pr_post_up_page_join_print(&route, &call, &print_report) ==
          PR_POST_UP_PAGE_JOIN_ORDER);
    return 0;
}

static int test_distinct_page_targets_and_boundaries(void) {
    uint8_t *plane[4] = {storage[0], storage[1], storage[2], storage[3]};
    pr_post_up_page_join route;
    pr_font201 font;
    pr_post_up_page_print_call call;
    pr_post_up_page_print_report report;

    reset_storage();
    CHECK(pr_font201_open(font_bytes, sizeof font_bytes, &font) == PR_FONT201_OK);
    CHECK(pr_post_up_page_join_open(&route, plane, PLANE_BYTES - 1u, 0u,
                                    0xA000u) == PR_POST_UP_PAGE_JOIN_BOUNDARY);
    CHECK(pr_post_up_page_join_open(&route, plane, PLANE_BYTES, 0u,
                                    0xA000u) == PR_POST_UP_PAGE_JOIN_OK);
    call = (pr_post_up_page_print_call){
        &font, "A", 1u, 17u, 15u, 10u, {0xA000u, 0xA400u}
    };
    CHECK(pr_post_up_page_join_print(&route, &call, &report) ==
          PR_POST_UP_PAGE_JOIN_BOUNDARY);
    CHECK(route.phase == PR_POST_UP_PAGE_FIRST_PRINT);
    CHECK(storage[1][ROW15_AT] == 0xA5u);
    call.draw_segment[1] = 0xA200u;
    CHECK(pr_post_up_page_join_print(&route, &call, &report) ==
          PR_POST_UP_PAGE_JOIN_OK);
    CHECK(report.distinct_pages == 2u);
    CHECK(report.page[0].cells == 1u && report.page[1].cells == 1u);
    CHECK(storage[1][ROW15_AT] == 0x3Cu);
    CHECK(storage[1][PAGE_STRIDE + ROW15_AT] == 0x3Cu);
    return 0;
}

int main(void) {
    CHECK(test_reached_page_value_route() == 0);
    CHECK(test_distinct_page_targets_and_boundaries() == 0);
    return 0;
}
