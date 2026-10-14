#ifndef POOLRAD_RECOVERED_SLOT_A_U3B0_PROMPT_ROW_H
#define POOLRAD_RECOVERED_SLOT_A_U3B0_PROMPT_ROW_H

#include "poolrad/recovered/slot_a_printer_page.h"

#include <stddef.h>
#include <stdint.h>

typedef enum {
    PR_SLOT_A_U3B0_ROW_OK = 0,
    PR_SLOT_A_U3B0_ROW_ARGUMENT,
    PR_SLOT_A_U3B0_ROW_BOUNDS,
    PR_SLOT_A_U3B0_ROW_DRAW
} pr_slot_a_u3b0_row_status;

typedef struct {
    uint8_t first_column;
    uint8_t length;
    const uint8_t *characters; /* Pascal payload, without its length byte. */
    const uint8_t *special_set; /* 32-byte set tested by SYSTEM +0x0DC0. */
    uint8_t selected_first;    /* Inclusive, one-based character positions. */
    uint8_t selected_last;
    uint8_t selected_color;
    uint8_t ordinary_color;
} pr_slot_a_u3b0_row_input;

/* Bounded page effect of unit 0x3B0:+0x01DA..+0x037C when called at +0x048B.
 * The caller supplies the copied Pascal characters, selection span, colors,
 * and 256-bit set from its live frame. Only the row-24 EGA page is modeled;
 * stack, clock, and key state are outside this boundary. Inputs whose cells
 * would run past column 38 are refused before any page write. */
pr_slot_a_u3b0_row_status pr_slot_a_u3b0_prompt_row_apply(
    uint8_t *page[4], const pr_font201 *font,
    const pr_slot_a_u3b0_row_input *input,
    pr_slot_a_printer_page_report *report);

#endif
