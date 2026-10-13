# 8X8D block 201: the DOS text font and its EGA glyph route (first slot-A view)

**Scope.** The font/text dependency of the first slot-A static view: the format of block
id 201 in `8X8D1.DAX`, the routine that loads it, and the glyph and string drawers that
consume it, all from the DOS v1.3 files (`8X8D1.DAX` sha256
`cce9313f23a600200fec60b5d46cd08aa9535a6143eee5d5d768a1d4e0415021`, 9,711 bytes;
`START.EXE` sha256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` sha256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`), the
resident image expanded in memory with the merged loader module. Only offsets, lengths,
counts and hashes appear here; the glyph bitmaps stay under ignored `resources/`. `deja`
recall for prior 8X8D/font work returned no session. Owner assignment: Dunking Sheep inbox
message i1200 (p2 owns this slice; p5 owns unit 0x3BD entry 5 tile selection; p6 owns
palette/display state). C: `src/recovered/font_8x8d_201.c`,
`include/poolrad/recovered/font_8x8d_201.h`, `tests/recovered/test_font_8x8d_201.c`.

## 1. The block

`8X8D1.DAX` holds six entries: ids 1, 3, 5 (2,257 bytes each, the 8x8 EGA tile sheets that
the merged EGA reader accepts), 201 (1,416 bytes unpacked, 1,341 packed), 202 (1,009) and
203 (1,457). Blocks 202 and 203 carry the 17-byte EGA header (height 8, one 8-pixel unit,
31 and 45 items: 17 + 31 x 32 = 1,009, 17 + 45 x 32 = 1,457). Block 201 does not: 1,416 is
not 17 plus a multiple of 32, but it is exactly 177 x 8. Block 201 appears in no other
`8X8D<n>.DAX` file. The unpacked block hashes to sha256
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.

Reading it as 177 glyphs of 8 bytes, one bit per pixel, bit 7 leftmost, rows top to
bottom: 8 glyphs are blank (indices 32, 95, 96, 97, 120, 121, 148 and 167; index 32 is the
space of the 6-bit text mapping), 156 of 177 glyphs end in a zero row (a blank descender
line), 27.9% of all bits are set. These are measurements; the layout itself is fixed by the
engine code in sections 2 and 3.

## 2. Load route (resident, decoded read-only)

| step | where | what the bytes show |
| --- | --- | --- |
| buffer | unit 0x36 + 0x0074..+0x0080 (the window allocator, FUNCTION_LEDGER.md 1.7) | `GetMem(0x588)` = 1,416 bytes into the far pointer `[0x5E20]`; the only overlay writer of `[0x5E20]` |
| call | unit 0x36 + 0x0481 | `0x709:0x017C` (`RES_0709_017C`, image `0x0720C`), the only call site of that routine |
| name | image `0x07212..0x0722B` | the 5-character constant at image `0x07201` and the 4-character constant at `0x07207` loaded as two strings (the file base name and extension, lower-case in the constant; DOS file names are case-insensitive) |
| request | image `0x07230..0x0723D` | `mov al, 0xC9` (201), `&size`, `&pointer`, `0x802:0x0716` (the resident DAX loader of ECL_BLOCK_LOAD.md section 3) |
| copy | image `0x07242..0x07258` | if size is non-zero: `Move(block, [0x5E20]^, size)` through `0xAF8:0x1685`; no prefix skip |
| release | image `0x0725D..0x07266` | `FreeMem(block, size)` through `0xAF8:0x0254`; `retf` |

So the buffer at `[0x5E20]` holds the raw unpacked block 201 from offset 0, and its size
equals the block size exactly.

## 3. Glyph drawer (resident `0x709:0x01DF`, `RES_0709_01DF`, image `0x0726F..0x0767A`, 1,036 bytes, 403 instructions, `retf 0xC`)

Arguments, first pushed to last: column `[bp+0x10]`, row `[bp+0xE]`, color `[bp+0xC]`,
count `[bp+0xA]`, character `[bp+8]`, upcase flag `[bp+6]`.

- `0x07275..0x0728A`: nothing when column > 0x27 or row > 0x18 or `[0x5E20]` is nil.
- `0x0728D..0x072AD`: glyph = upcase flag ? `UpCase(ch)` (`0xAF8:0x182A`) mod 64 (`idiv` by
  0x40, remainder) : the byte itself.
- `0x072B0..0x072C8`, `0x07667..0x07672`: count below 1 draws nothing; otherwise the cell is
  drawn count times, the column copy incremented after each.
- `0x072C8..0x072CF`: display mode byte `[0x4A1E]`: 0 takes the CGA path (`0x072D2..0x07414`:
  colors remapped through the 16-byte table at DS:0x25D5, 2-bit pixel words from the tables
  at DS:0x25CD and DS:0x25E5, written to segments B800 and BA00), 1 the EGA path, 2 the
  four-bank path (`0x0750B..0x07664`: segments B800, BA00, BC00, BE00, 160 bytes per line,
  nibble pairs). Only the EGA path is implemented in C; the other two are documented
  boundaries.
- EGA path `0x07417..0x07501`: byte offset = row x 8 x 40 + column; `out 0x3C4, 2` selects the
  map-mask register; for each of the 8 glyph rows and for each plane p = 0..3 the map mask
  is `DS:0x25C9[3 - p]` (the table holds 8, 4, 2, 1); when `color & mask` is non-zero the glyph
  row byte `[0x5E20] + glyph x 8 + row` is written, else a zero byte; the same byte goes to
  both page segments `[0x4A16]` and `[0x4A18]`; after the four planes the offset advances by
  0x28; at the end the map mask is restored to 0x0F. Hence a set bit yields the pixel index
  `color & 0x0F`, a clear bit yields 0, and all 64 pixels of the cell are written.

## 4. String printer (resident `0x709:0x0626`, `RES_0709_0626`, image `0x076B6..0x07736`, 129 bytes, 51 instructions, `retf 0xA`)

Arguments: far string `[bp+6]`, color `[bp+0xA]`, row `[bp+0xC]`, column `[bp+0xE]`.
`0x076BD..0x076CC` copies the string with limit 255; `0x076D1..0x076DD` returns when column
or row exceeds 0x27; `0x076EF..0x0772F` loops i = 1..length calling the drawer with
(column, row, color, 1, character i, upcase 1) and incrementing the column. 142
fixup-confirmed overlay sites in 39 overlay routines call it; the drawer has 10 direct
overlay sites in 4 routines (some with the upcase flag clear, using raw glyph indices
above 63).

## 5. The C and its tests

`pr_font201_open` accepts exactly 1,416 bytes; `pr_font201_glyph_index` is the upcase-mod-64
rule; `pr_font201_glyph_row` reads a glyph row; `pr_font201_draw_glyph_ega` writes one cell
of the EGA path into a `pr_slot_a_view_frame` (index `color & 0x0F` or 0, all 64 pixels
covered); `pr_font201_draw_ega` applies the count loop and `pr_font201_print_ega` the
printer's loop and guards. Byte-column wrap contract: the drawer's count loop increments
its byte column copy at image 0x7667 and the printer its byte column at 0x7724, so both
wrap from 255 to 0. The engine checks the column only on entry (once per drawer call) and
then writes past the row for columns 40 and up; this bounded model refuses every cell whose
byte column is 40..255 (BOUNDS from the count loop, SKIPPED from the printer, `drawn`
counting the cells made) and, like the engine's byte arithmetic, draws at 0..39 again after
the wrap at 256. That refusal is a stated divergence. Other deviations: glyph indices at or
above 177 are refused (the engine would read past its buffer); the two page segments are
one frame. Tests: block size, the index rule for all
256 bytes, glyph rows and bounds, one cell against a synthetic font (set and clear bits,
color bits above 0x0F ignored, the guard corners), the count loop including a count of 255
from column 30 (39 cells: 30..39 and, after the byte wrap, 0..28), and the printer (upcase,
empty string, both guards, the 255-character limit, cells past column 39 skipped, 230
characters from column 30 drawing 14 cells with the wrap landing the 227th character in
column 0).

## 6. Private parity audit on the owner's files (counts only)

Through the merged DAX reader and this C: block 201 opens; printing a 10-character
upper-case string at (0, 0) in color 15 drew 10 cells covering 640 pixels, of which
273 are non-zero; a second 10-character row of digits at (0, 1) drew 10 cells, bringing
the coverage to 1280 pixels; the reader counts 8 blank glyphs and reports the space glyph
blank. (A first version of this paragraph read the counts before the call that produced
them and said 0; corrected.) No native screen capture is compared here; the parity is
instruction-level against the EGA path.

## 7. Unresolved boundaries

The CGA and four-bank paths; the display mode byte `[0x4A1E]` and the page segments
`[0x4A16]`/`[0x4A18]` (p6's palette/display state); which strings the first slot-A view
prints and where (the callers of the printer are overlay routines outside this slice);
glyphs 64..176 as used by the raw-index callers.
