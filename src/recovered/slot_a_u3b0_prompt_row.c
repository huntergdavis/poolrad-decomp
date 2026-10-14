#include "poolrad/recovered/slot_a_u3b0_prompt_row.h"

#include <string.h>

pr_slot_a_u3b0_row_status pr_slot_a_u3b0_prompt_row_apply(
    uint8_t *page[4], const pr_font201 *font,
    const pr_slot_a_u3b0_row_input *input,
    pr_slot_a_printer_page_report *report) {
    size_t i, end, blank_count;
    pr_slot_a_printer_page_report one;
    if (report == NULL) {
        return PR_SLOT_A_U3B0_ROW_ARGUMENT;
    }
    *report = (pr_slot_a_printer_page_report){0};
    if (page == NULL || font == NULL || font->data == NULL ||
        font->size != PR_FONT201_BLOCK_SIZE || input == NULL ||
        input->special_set == NULL ||
        (input->length != 0u && input->characters == NULL)) {
        return PR_SLOT_A_U3B0_ROW_ARGUMENT;
    }
    for (i = 0u; i < 4u; ++i) {
        if (page[i] == NULL) {
            return PR_SLOT_A_U3B0_ROW_ARGUMENT;
        }
    }
    if (input->length == 0u) {
        return PR_SLOT_A_U3B0_ROW_OK;
    }
    end = (size_t)input->first_column + input->length;
    if (input->first_column > 39u || end > 39u ||
        input->selected_color > 15u || input->ordinary_color > 15u) {
        return PR_SLOT_A_U3B0_ROW_BOUNDS;
    }
    for (i = 1u; i <= input->length; ++i) {
        uint8_t ch = input->characters[i - 1u];
        uint8_t in_set = (uint8_t)(input->special_set[ch >> 3u] &
                                   (uint8_t)(1u << (ch & 7u)));
        uint8_t color = input->ordinary_color;
        char glyph = (char)ch;
        if ((i >= input->selected_first && i <= input->selected_last &&
             input->selected_color != 0u) || in_set != 0u) {
            color = input->selected_color;
        }
        if (pr_slot_a_printer_page_apply(page, 0u, 8000u, font,
                                         (uint8_t)(input->first_column + i - 1u),
                                         24u, color, &glyph, 1u, &one) !=
            PR_SLOT_A_PRINTER_PAGE_OK) {
            return PR_SLOT_A_U3B0_ROW_DRAW;
        }
        report->cells += one.cells;
        report->plane_stores += one.plane_stores;
        report->changed_plane_bytes += one.changed_plane_bytes;
    }
    /* The original tests end < 39, then writes spaces through column 38. */
    blank_count = end < 39u ? 39u - end : 0u;
    if (blank_count != 0u) {
        char blanks[39];
        memset(blanks, ' ', sizeof blanks);
        if (pr_slot_a_printer_page_apply(page, 0u, 8000u, font,
                                         (uint8_t)end, 24u, 0u,
                                         blanks, blank_count, &one) !=
            PR_SLOT_A_PRINTER_PAGE_OK) {
            return PR_SLOT_A_U3B0_ROW_DRAW;
        }
        report->cells += one.cells;
        report->plane_stores += one.plane_stores;
        report->changed_plane_bytes += one.changed_plane_bytes;
    }
    return PR_SLOT_A_U3B0_ROW_OK;
}
