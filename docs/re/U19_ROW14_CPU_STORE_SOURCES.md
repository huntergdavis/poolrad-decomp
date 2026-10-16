# U19 row-14 glyph: ordered CPU store sources

This bounded C composition emits the **CPU source operands** of one accepted
printable glyph in U19's fixed row-14 printer call. It does not execute EGA
memory semantics or establish a live first writer. A bounded
`deja "Poolrad U19 row14 printer drawer ordered plane page CPU source bytes C
model glyph 0412 0438 0445 0451"` search did not return before its stop.
The merged [store candidate audit](COLD_START_CODEWORD_ROW14_STORE_CANDIDATES.md),
[font contract](FONT_8X8D_201.md), and
[CPU-store capture gate](COLD_START_CODEWORD_ROW14_CPU_STORE_GATE.md)
supplied the route, glyph source, and four store sites; each original span
was checked again against ignored START.EXE and the retained RAM.

## Original-byte boundary

Ignored DOS v1.3 START.EXE SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The original drawer EGA path `RES_0709:+0387..+0470`, image
`[0x7417,0x7501)`, has 234 bytes and SHA-256
`3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4`.
The checked-in hash-locked candidate audit verifies the `+03ED` color-zero
branch and four original store instructions. A retained DS=`14A0`
four-byte map-mask table has SHA-256
`1ba1475eadf8ae7a3b170515ef3c887513a6feed761b5f6f889882b809cd553f`;
indexing it as `[3 - plane]` yields masks 1, 2, 4, 8 in loop order.

At `+0395..+03A5`, the byte address is
`(text_row * 8 * 40) + column`; `+045E` adds 40 after each of eight
glyph rows. At each row, `+03BB..+045B` loops plane indexes 0..3.
If `color & mask` is nonzero, the drawer rereads the glyph byte from
`[DS:5E20] + glyph*8 + glyph_row` for each page. Otherwise it writes
immediate zero. The first page segment comes from DS:4A16 and the second
from DS:4A18. Both stores have the same byte offset.

| Color branch | First page | Second page | CPU source |
| --- | --- | --- | --- |
| nonzero | `+0412` | `+0438` | font byte in DL |
| zero | `+0445` | `+0451` | immediate zero |

## C boundary

`pr_u19_row14_cpu_sources` composes the merged
`pr_slot_a_glyph_plane_byte` helper. It accepts a stable synthetic or
supplied 1,416-byte
font block, character and upcase flag, byte column 0..39, color, and two
page-segment words. It emits exactly 8 rows × 4 plane indexes × 2 page
stores = **64 ordered events**. Each records the instruction offset,
segment:offset, row, plane/mask, page order, source byte, and whether the
source is a font read or immediate zero. It rejects an invalid font, glyph,
column, or output capacity before writing an event. Caller-owned font and
event storage must not alias.

`pr_u19_row14_fixed_sources` composes this one-glyph model over exactly
36 caller-supplied cells at columns 1..36, with UpCase enabled and color
10, yielding **2,304** ordered CPU source events. The fixed text bytes
remain private; synthetic tests use invented bytes. The model assumes the
EGA branch selected at `+0387` (AL=1). For the retained
`(x,y)=(122,112)` witness, column 15 gives byte offset `0x118F`.
Color 10 takes zero, glyph, zero, glyph branches for plane indexes 0..3.
The synthetic test checks all 64 positions within a glyph and the fixed
row's first, witness, and last event positions, both page segments,
the four sites, and failure without partial output.
It also distinguishes a zero-valued font byte on the glyph path from an
immediate-zero store. The C model does **not** know the initial plane bytes
or the Graphics Controller latch/write mode, so its source bytes do not
predict the resulting four-plane bytes.

Reproduce the source audit and focused strict C test:

```sh
python3 tools/replay/cold_start_codeword_row14_store_candidate_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --run resources/oracle-runs/codeword-row14-p5-001
ctest --test-dir resources/build-u19-row14-cpu-sources-p5 \
  -R '^poolrad_u19_row14_cpu_sources_tests$' --output-on-failure
```

The branch contains only address/hash/count evidence and synthetic inputs.
It gives no full printer or glyph-drawer translation, first value-changing
EGA writer, page selection, host RGB timing, native parity, full frame,
strict full-C, or identity credit. Original bytes, RAM, font, and pixels
remain under ignored `resources/`.
