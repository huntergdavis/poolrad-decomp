#include "poolrad/recovered/slot_a_glyph_plane_byte.h"

pr_font201_status pr_slot_a_glyph_plane_byte(const pr_font201 *font,
                                               uint8_t ch, int upcase,
                                               uint8_t glyph_row, uint8_t color,
                                               unsigned plane, uint8_t *out) {
    uint8_t bits;
    pr_font201_status status;
    if (out == NULL || plane >= 4u) {
        return PR_FONT201_ARGUMENT;
    }
    status = pr_font201_glyph_row(font, pr_font201_glyph_index(ch, upcase),
                                  glyph_row, &bits);
    if (status != PR_FONT201_OK) {
        return status;
    }
    *out = (color & (uint8_t)(1u << plane)) != 0u ? bits : 0u;
    return PR_FONT201_OK;
}
