#include "poolrad/recovered/slot_a_post_up_page_join.h"

#include <string.h>

enum { PAGE_BYTES = 8000, PAGE_STRIDE = 8192, TWO_PAGE_BYTES = 16192 };

static int page_index(uint16_t segment) {
    if (segment == 0xA000u) return 0;
    if (segment == 0xA200u) return 1;
    return -1;
}

pr_post_up_page_join_status pr_post_up_page_join_open(
    pr_post_up_page_join *route, uint8_t *plane[4], size_t plane_bytes,
    uint8_t active_page, uint16_t draw_segment) {
    size_t lane;
    if (route == NULL || plane == NULL) return PR_POST_UP_PAGE_JOIN_ARGUMENT;
    for (lane = 0u; lane < 4u; ++lane) {
        if (plane[lane] == NULL) return PR_POST_UP_PAGE_JOIN_ARGUMENT;
    }
    if (plane_bytes < TWO_PAGE_BYTES || active_page != 0u ||
        draw_segment != 0xA000u) return PR_POST_UP_PAGE_JOIN_BOUNDARY;
    memset(route, 0, sizeof *route);
    for (lane = 0u; lane < 4u; ++lane) route->video.plane[lane] = plane[lane];
    route->video.plane_bytes = plane_bytes;
    route->video.active_page = active_page;
    route->video.draw_segment = draw_segment;
    route->phase = PR_POST_UP_PAGE_FIRST_PRINT;
    return PR_POST_UP_PAGE_JOIN_OK;
}

pr_post_up_page_join_status pr_post_up_page_join_print(
    pr_post_up_page_join *route, const pr_post_up_page_print_call *call,
    pr_post_up_page_print_report *report) {
    uint8_t *page[4];
    int index[2];
    size_t target, lane;
    if (report == NULL) return PR_POST_UP_PAGE_JOIN_ARGUMENT;
    memset(report, 0, sizeof *report);
    if (route == NULL || call == NULL || call->font == NULL ||
        (call->text == NULL && call->length != 0u)) {
        return PR_POST_UP_PAGE_JOIN_ARGUMENT;
    }
    if (route->phase != PR_POST_UP_PAGE_FIRST_PRINT &&
        route->phase != PR_POST_UP_PAGE_SECOND_PRINT) {
        return PR_POST_UP_PAGE_JOIN_ORDER;
    }
    index[0] = page_index(call->draw_segment[0]);
    index[1] = page_index(call->draw_segment[1]);
    if (index[0] < 0 || index[1] < 0 ||
        route->video.plane_bytes < TWO_PAGE_BYTES) {
        return PR_POST_UP_PAGE_JOIN_BOUNDARY;
    }
    for (lane = 0u; lane < 4u; ++lane) {
        if (route->video.plane[lane] == NULL) return PR_POST_UP_PAGE_JOIN_ARGUMENT;
    }
    for (target = 0u; target < 2u; ++target) {
        pr_slot_a_printer_page_status status;
        int page_number = index[target];
        if (target == 1u && page_number == index[0]) continue;
        for (lane = 0u; lane < 4u; ++lane) {
            page[lane] = route->video.plane[lane] +
                         (size_t)page_number * PAGE_STRIDE;
        }
        status = pr_slot_a_printer_page_apply(
            page, 0u, PAGE_BYTES, call->font, call->column, call->row,
            call->color, call->text, call->length,
            &report->page[page_number]);
        if (status != PR_SLOT_A_PRINTER_PAGE_OK) {
            return status == PR_SLOT_A_PRINTER_PAGE_ARGUMENT ?
                PR_POST_UP_PAGE_JOIN_ARGUMENT : PR_POST_UP_PAGE_JOIN_BOUNDARY;
        }
        ++report->distinct_pages;
    }
    route->phase = route->phase == PR_POST_UP_PAGE_FIRST_PRINT ?
        PR_POST_UP_PAGE_ROW15_CLEAR : PR_POST_UP_PAGE_DONE;
    return PR_POST_UP_PAGE_JOIN_OK;
}

pr_post_up_page_join_status pr_post_up_page_join_clear(
    pr_post_up_page_join *route, pr_res0709_rect_args args,
    pr_res0709_rect_report *report) {
    pr_res0709_rect_status status;
    if (report == NULL) return PR_POST_UP_PAGE_JOIN_ARGUMENT;
    memset(report, 0, sizeof *report);
    if (route == NULL) return PR_POST_UP_PAGE_JOIN_ARGUMENT;
    if (route->phase != PR_POST_UP_PAGE_ROW15_CLEAR) {
        return PR_POST_UP_PAGE_JOIN_ORDER;
    }
    if (args.color != 0u || args.page != 0u || args.bottom != 15u ||
        args.right != 38u || args.top != 15u || args.left != 17u ||
        route->video.active_page != 0u) {
        return PR_POST_UP_PAGE_JOIN_BOUNDARY;
    }
    status = pr_res0709_rect_fill_00b7(&route->video, args, report);
    if (status != PR_RES0709_RECT_OK) {
        return status == PR_RES0709_RECT_ARGUMENT ?
            PR_POST_UP_PAGE_JOIN_ARGUMENT : PR_POST_UP_PAGE_JOIN_BOUNDARY;
    }
    route->phase = PR_POST_UP_PAGE_SECOND_PRINT;
    return PR_POST_UP_PAGE_JOIN_OK;
}
