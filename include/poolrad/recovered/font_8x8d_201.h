#ifndef POOLRAD_RECOVERED_FONT_8X8D_201_H
#define POOLRAD_RECOVERED_FONT_8X8D_201_H

#include "poolrad/replay/slot_a_static_view.h"

#include <stddef.h>
#include <stdint.h>

/* The DOS v1.3 text font: block id 201 of 8X8D1.DAX (unpacked 1,416 bytes),
 * 177 glyphs of 8 bytes, one bit per pixel, bit 7 the leftmost pixel, rows
 * top to bottom. Evidence (docs/re/FONT_8X8D_201.md): the window allocator
 * (unit 0x36 + 0x002D) allocates exactly 0x588 bytes at +0x0074 for the far
 * pointer [0x5E20]; resident 0x709:0x017C builds the file name from the two
 * constants at image 0x7201 (5 characters) and 0x7207 (4 characters),
 * requests id 0xC9 from the DAX loader 0x802:0x0716, copies the whole block
 * into [0x5E20] and releases it; the glyph drawer 0x709:0x01DF reads
 * [0x5E20] + glyph * 8 + row and tests bits from 0x80 downward. */
#define PR_FONT201_BLOCK_ID 201u
#define PR_FONT201_BLOCK_SIZE 1416u        /* GetMem(0x588) */
#define PR_FONT201_GLYPH_BYTES 8u
#define PR_FONT201_GLYPH_COUNT (PR_FONT201_BLOCK_SIZE / PR_FONT201_GLYPH_BYTES)   /* 177 */
#define PR_FONT201_COLUMNS 40u             /* drawer guard: column <= 0x27 */
#define PR_FONT201_ROWS 25u                /* drawer guard: row <= 0x18 */
#define PR_FONT201_PRINT_LIMIT 40u         /* printer guard: column and row <= 0x27 */

typedef enum {
    PR_FONT201_OK = 0,
    PR_FONT201_ARGUMENT,
    PR_FONT201_SIZE,          /* block is not 1,416 bytes */
    PR_FONT201_GLYPH,         /* glyph index past the 177 glyphs (the engine would read past its buffer) */
    PR_FONT201_BOUNDS,        /* column or row outside the drawer's guard on entry (the engine draws
                               * nothing), or, in the count loop, at least one copy refused because
                               * its byte column lay in 40..255 (see the wrap contract below) */
    PR_FONT201_SKIPPED        /* printer: one or more cells were not drawn, either because the drawer's
                               * guard rejected them (row > 24) or because their byte column lay in
                               * 40..255 (see the wrap contract below); `drawn` counts the cells made */
} pr_font201_status;

typedef struct {
    const uint8_t *data;      /* borrowed block, kept alive by the caller */
    size_t size;
} pr_font201;

pr_font201_status pr_font201_open(const uint8_t *data, size_t size, pr_font201 *out);

/* 0x709:0x01DF +0x1E..+0x41: with the upcase flag the glyph is UpCase(ch) mod
 * 64 (0xAF8:0x182A then idiv by 0x40); without it the byte is the glyph. */
uint8_t pr_font201_glyph_index(uint8_t ch, int upcase);

pr_font201_status pr_font201_glyph_row(const pr_font201 *font, size_t glyph, size_t row,
                                       uint8_t *bits);

/* One 8x8 cell of the EGA path (display mode byte [0x4A1E] == 1) of
 * 0x709:0x01DF at image 0x7417..0x7501: for each row and each plane the glyph
 * row byte is written when the color has that plane's bit (table DS:0x25C9 =
 * 8, 4, 2, 1), else zero; so a set bit becomes `color & 0x0F` and a clear bit
 * becomes 0, and every pixel of the cell is written (opaque). Both page
 * segments [0x4A16] and [0x4A18] receive the same bytes; the frame stands
 * for the page. BOUNDS when column > 39 or row > 24 (the engine draws nothing). */
pr_font201_status pr_font201_draw_glyph_ega(pr_slot_a_view_frame *frame, const pr_font201 *font,
                                            uint8_t column, uint8_t row, uint8_t color,
                                            size_t glyph);

/* 0x709:0x01DF as called: `count` copies of the glyph for `ch` at successive
 * columns; the drawer increments its BYTE column copy at image 0x7667, so
 * the column wraps from 255 to 0. Byte-column wrap contract of this bounded
 * model: the engine checks the column only on entry and then writes past the
 * row for columns 40 and up; the model refuses every copy whose byte column
 * is 40..255 (reported as BOUNDS with `drawn` counting the copies made) and,
 * like the engine's byte arithmetic, lets columns 0..39 draw again after the
 * wrap at 256. That refusal is a stated divergence from the original. */
pr_font201_status pr_font201_draw_ega(pr_slot_a_view_frame *frame, const pr_font201 *font,
                                      uint8_t column, uint8_t row, uint8_t color,
                                      uint8_t count, uint8_t ch, int upcase, size_t *drawn);

/* 0x709:0x0626 (image 0x76B6..0x7736): a Pascal string of at most 255
 * characters printed at (column, row) in `color`, each character through the
 * drawer with count 1 and the upcase flag, the column advanced per character.
 * The printer's own guard is column <= 39 and row <= 39 (BOUNDS); cells the
 * drawer then rejects (row > 24, column past 39) are skipped as the engine
 * does, reported as SKIPPED with `drawn` holding the cells written. The
 * printer increments its BYTE column at image 0x7724, so a string long
 * enough wraps the column from 255 to 0 and characters from there on draw
 * at columns 0..39 again, in the engine and in this model alike. */
pr_font201_status pr_font201_print_ega(pr_slot_a_view_frame *frame, const pr_font201 *font,
                                       uint8_t column, uint8_t row, uint8_t color,
                                       const char *text, size_t len, size_t *drawn);

#endif
