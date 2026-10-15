# RES_0709:01DF second glyph drawer and final printer return

This bounded C model starts at the second `RES_0709:01DF` glyph call on a
two-character synthetic Pascal string. It follows the EGA mode-one path
through the drawer's `RETF 0C` at `05E8`, stops at printer `0709:0694`,
then models the printer's `[0694,06A7)` final loop check and `RETF 0A` to
`RES_00BA:1858`. The original-byte fixture starts at each of the eight
pinned overlay callers and reaches these stops in one run. The C comparison
begins at the second drawer entry; the earlier caller, display, copy, first
glyph, and printer loop are original-byte context.

## Pinned inputs and frame

The ignored original `START.EXE` is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Its expanded resident drawer image `[0x726F,0x767B)` is **1,036 bytes**,
SHA-256 `a9d1c72241b6810c3e0995b03cca98edc56cb76734e948333e360d82ef7e1840`.
The final byte is the high immediate byte of `RETF 0C`, so the return is
complete. The original SYSTEM upcase helper, raw START
`[0xC4A0,0xC4B3)`, is 19 bytes, SHA-256
`738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c`.
The ignored `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The prior [post-copy control gate](RESIDENT_0709_0626_POSTCOPY_CONTROL.md)
pins the original printer and Pascal copy helper used to reach this entry.
The test checks those hashes again at runtime.

At second entry, the far-call stack begins with return `0694:0709`,
upcase flag 1, the second character, cell count 1, color, row, and column.
The test varies lower-case `t`, `b`, and `a`; original SYSTEM `182A`
uppercases them, and the drawer's signed divide by `0x40` yields glyph
indices `0x14`, `0x02`, and `0x01`. The synthetic font supplies eight
distinct scanline bytes for each glyph. In mode one, the drawer checks
column `<=39`, row `<=24`, and a nonzero font pointer, then calculates
`row*320+column`. For each of eight scanlines, planes 0 through 3 select
the byte at `DS:25C9+[3-plane]`, mask it with the color, and write either
the font byte or zero to both `DS:4A16` and `DS:4A18` page segments.
It advances the page offset by 40 after every scanline, writes `0F` to
port `03C5`, increments the drawer's column argument, and returns to
`0694`. The printer increments its own column byte, compares one-based
index 2 against copied count 2, and returns to `00BA:1858`.

## Differential result and limits

**40/40** cases match all 1 MiB flat RAM bytes and all 14 CPU fields at
both the second drawer and final printer returns. This comprises eight
overlay sites and five synthetic variants each. Three variants vary the
incoming color and glyph through the original caller. Two further
variants override the second drawer's color argument at the entry
boundary with masks `00` and `01`; they exercise all-clear writes and an
early selected plane. The original-byte trace verifies **2,560/2,560**
flat A000 write events: 64 in each case, with exact address, width,
value, and order. Arithmetic flags use the existing CPU comparison mask,
which excludes undefined AF after reached XOR instructions. The model
compiles with `-std=c11 -Wall -Wextra -Wpedantic -Werror`.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_0709_second_glyph_return.py
```

Unicorn's flat A000 memory does not emulate hardware EGA planes or port
side effects. The mask bytes, font, page segments, characters, and rows
are synthetic. This model covers the reached mode-one, one-cell drawer
and the two-character printer exit; other video modes, multi-cell drawer
loops, invalid argument paths, and the remaining `RES_00BA:1816`
continuation remain outside this gate. No strict full-C or playable-game
credit follows from these bounded comparisons.

Before this gate, bounded
`deja "Poolrad RES_0709 01DF second glyph EGA drawer return 0694 06A1"`
timed out without an indexed result; no prior-session assertion was reused.
The original bytes and prior [clean-page glyph watch](SLOT_A_FIRST_CLEAN_PAGE_GLYPH_WRITE.md)
were independently rechecked. Original game media and captures remain
under ignored `resources/`; this public evidence records only addresses,
hashes, counts, and synthetic data.
