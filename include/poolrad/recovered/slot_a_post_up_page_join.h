#ifndef POOLRAD_RECOVERED_SLOT_A_POST_UP_PAGE_JOIN_H
#define POOLRAD_RECOVERED_SLOT_A_POST_UP_PAGE_JOIN_H

#include "poolrad/recovered/resident_0709_rect_fill_00b7.h"
#include "poolrad/recovered/slot_a_printer_page.h"

#include <stddef.h>
#include <stdint.h>

typedef enum {
    PR_POST_UP_PAGE_JOIN_OK = 0,
    PR_POST_UP_PAGE_JOIN_ARGUMENT,
    PR_POST_UP_PAGE_JOIN_ORDER,
    PR_POST_UP_PAGE_JOIN_BOUNDARY
} pr_post_up_page_join_status;

typedef enum {
    PR_POST_UP_PAGE_FIRST_PRINT = 0,
    PR_POST_UP_PAGE_ROW15_CLEAR,
    PR_POST_UP_PAGE_SECOND_PRINT,
    PR_POST_UP_PAGE_DONE
} pr_post_up_page_phase;

typedef struct {
    pr_res0709_rect_state video;
    pr_post_up_page_phase phase;
} pr_post_up_page_join;

typedef struct {
    const pr_font201 *font;
    const char *text;
    size_t length;
    uint8_t column;
    uint8_t row;
    uint8_t color;
    uint16_t draw_segment[2]; /* live DS:4A16 and DS:4A18 */
} pr_post_up_page_print_call;

typedef struct {
    pr_slot_a_printer_page_report page[2];
    size_t distinct_pages;
} pr_post_up_page_print_report;

/* Page-value route only. Each supplied plane spans A000 page 0 at offsets
 * 0..7999 and A200 page 1 at offsets 8192..16191. The 192-byte gap is
 * caller-owned and preserved. Call frames and initial pages are supplied by
 * the caller; this interface does not dispatch Up or update party/world RAM. */
pr_post_up_page_join_status pr_post_up_page_join_open(
    pr_post_up_page_join *route, uint8_t *plane[4], size_t plane_bytes,
    uint8_t active_page, uint16_t draw_segment);

/* Call once at the first and second reached printer entries, in that order.
 * Repeated A000/A000 targets are composed once for page-value purposes; no
 * native guest store count is inferred from the report. */
pr_post_up_page_join_status pr_post_up_page_join_print(
    pr_post_up_page_join *route, const pr_post_up_page_print_call *call,
    pr_post_up_page_print_report *report);

/* The only modeled middle call is the measured color-0, page-0 rectangle
 * spanning row 15, columns 17..38. Other frames stop at BOUNDARY. */
pr_post_up_page_join_status pr_post_up_page_join_clear(
    pr_post_up_page_join *route, pr_res0709_rect_args args,
    pr_res0709_rect_report *report);

#endif
