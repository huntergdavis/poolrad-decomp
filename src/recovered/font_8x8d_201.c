#include "poolrad/recovered/font_8x8d_201.h"

pr_font201_status pr_font201_open(const uint8_t *data, size_t size, pr_font201 *out) {
    if (out == NULL) {
        return PR_FONT201_ARGUMENT;
    }
    out->data = NULL;
    out->size = 0u;
    if (data == NULL) {
        return PR_FONT201_ARGUMENT;
    }
    if (size != PR_FONT201_BLOCK_SIZE) {
        return PR_FONT201_SIZE;              /* the engine's buffer is GetMem(0x588) */
    }
    out->data = data;
    out->size = size;
    return PR_FONT201_OK;
}

uint8_t pr_font201_glyph_index(uint8_t ch, int upcase) {
    if (!upcase) {
        return ch;                           /* 0x72AA: mov al,[bp+8]; mov [bp-2],al */
    }
    if (ch >= (uint8_t)'a' && ch <= (uint8_t)'z') {
        ch = (uint8_t)(ch - 0x20u);          /* 0xAF8:0x182A, UpCase */
    }
    return (uint8_t)(ch % 64u);              /* 0x729C..0x72A5: cwd; idiv 0x40; remainder */
}

pr_font201_status pr_font201_glyph_row(const pr_font201 *font, size_t glyph, size_t row,
                                       uint8_t *bits) {
    if (font == NULL || font->data == NULL || bits == NULL || row >= PR_FONT201_GLYPH_BYTES) {
        return PR_FONT201_ARGUMENT;
    }
    if (glyph >= PR_FONT201_GLYPH_COUNT) {
        return PR_FONT201_GLYPH;
    }
    *bits = font->data[glyph * PR_FONT201_GLYPH_BYTES + row];   /* es:[[0x5E20] + glyph*8 + row] */
    return PR_FONT201_OK;
}

pr_font201_status pr_font201_draw_glyph_ega(pr_slot_a_view_frame *frame, const pr_font201 *font,
                                            uint8_t column, uint8_t row, uint8_t color,
                                            size_t glyph) {
    size_t r, x;
    if (frame == NULL || font == NULL || font->data == NULL) {
        return PR_FONT201_ARGUMENT;
    }
    if (column > 0x27u || row > 0x18u) {     /* 0x7275..0x728A */
        return PR_FONT201_BOUNDS;
    }
    if (glyph >= PR_FONT201_GLYPH_COUNT) {
        return PR_FONT201_GLYPH;
    }
    /* 0x741E..0x7435: byte offset = row * 8 * 40 + column; the frame uses
     * pixel coordinates (column * 8, row * 8). 0x7444..0x74F8: for each of the
     * 8 rows, for each plane (map mask from DS:0x25C9[3 - p]): glyph byte
     * when color & mask, else 0; then advance one line (0x28 bytes). */
    for (r = 0u; r < 8u; ++r) {
        uint8_t bits = font->data[glyph * PR_FONT201_GLYPH_BYTES + r];
        for (x = 0u; x < 8u; ++x) {
            size_t target = ((size_t)row * 8u + r) * PR_SLOT_A_VIEW_WIDTH + (size_t)column * 8u + x;
            uint8_t set = (uint8_t)((bits >> (7u - x)) & 1u);
            frame->index[target] = set ? (uint8_t)(color & 0x0Fu) : 0u;
            frame->covered[target] = 1u;
        }
    }
    return PR_FONT201_OK;                    /* 0x74FB: map mask restored to 0x0F */
}

pr_font201_status pr_font201_draw_ega(pr_slot_a_view_frame *frame, const pr_font201 *font,
                                      uint8_t column, uint8_t row, uint8_t color,
                                      uint8_t count, uint8_t ch, int upcase, size_t *drawn) {
    size_t glyph = pr_font201_glyph_index(ch, upcase);
    unsigned i;
    pr_font201_status last = PR_FONT201_OK;
    if (drawn != NULL) {
        *drawn = 0u;
    }
    if (frame == NULL || font == NULL || font->data == NULL) {
        return PR_FONT201_ARGUMENT;
    }
    if (column > 0x27u || row > 0x18u) {     /* the entry guard, once */
        return PR_FONT201_BOUNDS;
    }
    if (count == 0u) {
        return PR_FONT201_OK;                /* 0x72B6..0x72BD: a count below 1 draws nothing */
    }
    for (i = 0u; i < count; ++i) {           /* 0x72C0..0x7672: [bp-1] = 1..count, column++ each pass */
        pr_font201_status st = pr_font201_draw_glyph_ega(frame, font, (uint8_t)(column + i), row, color, glyph);
        if (st == PR_FONT201_OK) {
            if (drawn != NULL) {
                ++*drawn;
            }
        } else {
            last = st;                       /* model: byte columns 40..255 are refused; the (uint8_t) cast
                                              * wraps at 256 as the engine's byte increment does */
            if (st == PR_FONT201_GLYPH) {
                return st;
            }
        }
    }
    return last;
}

pr_font201_status pr_font201_print_ega(pr_slot_a_view_frame *frame, const pr_font201 *font,
                                       uint8_t column, uint8_t row, uint8_t color,
                                       const char *text, size_t len, size_t *drawn) {
    size_t i, made = 0u;
    pr_font201_status result = PR_FONT201_OK;
    if (drawn != NULL) {
        *drawn = 0u;
    }
    if (frame == NULL || font == NULL || font->data == NULL || (text == NULL && len != 0u)) {
        return PR_FONT201_ARGUMENT;
    }
    if (len > 255u) {
        len = 255u;                          /* 0x76BD..0x76CC: copied with limit 255 */
    }
    if (column > 0x27u || row > 0x27u) {     /* 0x76D1..0x76DD */
        return PR_FONT201_BOUNDS;
    }
    for (i = 0u; i < len; ++i) {             /* 0x76EF..0x772F: 1..length, column++ per character */
        pr_font201_status st = pr_font201_draw_ega(frame, font, (uint8_t)(column + i), row, color, 1u,
                                                   (uint8_t)text[i], 1, NULL);
        if (st == PR_FONT201_OK) {
            ++made;
        } else if (st == PR_FONT201_BOUNDS) {
            result = PR_FONT201_SKIPPED;     /* the drawer's guard: nothing drawn for that cell */
        } else {
            return st;
        }
    }
    if (drawn != NULL) {
        *drawn = made;
    }
    return result;
}
