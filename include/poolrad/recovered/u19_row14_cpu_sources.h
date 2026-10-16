#ifndef POOLRAD_RECOVERED_U19_ROW14_CPU_SOURCES_H
#define POOLRAD_RECOVERED_U19_ROW14_CPU_SOURCES_H

#include "poolrad/recovered/font_8x8d_201.h"

#define PR_U19_ROW14_CPU_EVENTS 64u
#define PR_U19_ROW14_FIXED_CELLS 36u
#define PR_U19_ROW14_FIXED_EVENTS (PR_U19_ROW14_CPU_EVENTS * PR_U19_ROW14_FIXED_CELLS)

/* One source operand for an EGA drawer MOV. These events describe CPU writes,
 * not the resulting EGA plane bytes. page=0 uses DS:4A16, page=1 DS:4A18. */
typedef struct {
    uint16_t store_offset;  /* loaded RES_0709 IP: +0412/0438/0445/0451 */
    uint16_t segment;
    uint16_t offset;
    uint8_t glyph_row;
    uint8_t plane;
    uint8_t page;
    uint8_t map_mask;
    uint8_t source_byte;
    uint8_t glyph_source;   /* 1: DL from font, 0: immediate zero */
} pr_u19_row14_cpu_event;

/* The 64 ordered CPU source operands for one accepted, printable glyph at
 * fixed text row 14. The DOS drawer runs eight glyph rows, four plane masks
 * (1,2,4,8), and first/draw page stores in that order. The font is stable
 * through this call; events and font storage must not alias. No EGA latch,
 * mask, page-selection, or host effect is modeled.
 *
 * Rejects invalid input without writing events. *count is zeroed when
 * supplied. column must be 0..39 and capacity at least 64. */
pr_font201_status pr_u19_row14_cpu_sources(
    const pr_font201 *font, uint8_t ch, int upcase, uint8_t column,
    uint8_t color, uint16_t active_segment, uint16_t draw_segment,
    pr_u19_row14_cpu_event *events, size_t capacity, size_t *count);

/* The U19 +02D8 fixed printer call uses column 1, row 14, color 10, and a
 * 36-character source. The caller supplies synthetic or private bytes;
 * none are embedded here. This composes 36 accepted glyphs in printer order
 * with UpCase enabled, producing 2,304 CPU events. No EGA effects. */
pr_font201_status pr_u19_row14_fixed_sources(
    const pr_font201 *font, const uint8_t *text, size_t length,
    uint16_t active_segment, uint16_t draw_segment,
    pr_u19_row14_cpu_event *events, size_t capacity, size_t *count);

#endif
