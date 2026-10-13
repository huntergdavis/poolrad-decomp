#ifndef POOLRAD_RECOVERED_SLOT_A_GLYPH_PLANE_BYTE_H
#define POOLRAD_RECOVERED_SLOT_A_GLYPH_PLANE_BYTE_H

#include "poolrad/recovered/font_8x8d_201.h"

/* The EGA byte written by RES_0709:0412 for one glyph scanline and plane.
 * Plane 0 is the low color bit; plane 3 is the high color bit. This models
 * one bounded store, not the complete glyph drawer or its call route. */
pr_font201_status pr_slot_a_glyph_plane_byte(const pr_font201 *font,
                                               uint8_t ch, int upcase,
                                               uint8_t glyph_row, uint8_t color,
                                               unsigned plane, uint8_t *out);

#endif
