#ifndef POOLRAD_RECOVERED_SLOT_A_SECOND_CYCLE_ZERO_ROW_H
#define POOLRAD_RECOVERED_SLOT_A_SECOND_CYCLE_ZERO_ROW_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    PR_SLOT_A_SECOND_ZERO_ROW_OK = 0,
    PR_SLOT_A_SECOND_ZERO_ROW_ARGUMENT,
    PR_SLOT_A_SECOND_ZERO_ROW_BOUNDARY
} pr_slot_a_second_zero_row_status;

typedef struct {
    size_t plane_stores;
    size_t changed_plane_bytes;
} pr_slot_a_second_zero_row_report;

/* Bounded EGA page effect of the inter-cycle row-four reset and the five
 * reached second-cycle row clears. Only color 0, rows 4..9, and inclusive
 * columns 17..38 are supported.
 * page[p][0] is video byte page_base; each plane has page_bytes. This models
 * the page result of the observed RES_03F1:0137 -> RES_0709:00B7 route,
 * not the complete rectangle routine or transient hardware state. */
pr_slot_a_second_zero_row_status pr_slot_a_second_zero_row_apply(
    uint8_t *page[4], size_t page_base, size_t page_bytes,
    uint8_t row, uint8_t left, uint8_t right, uint8_t color,
    pr_slot_a_second_zero_row_report *report);

#endif
