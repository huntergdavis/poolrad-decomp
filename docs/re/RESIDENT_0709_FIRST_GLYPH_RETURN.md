# RES_0709:0415 first EGA glyph continuation to printer 0694

This gate starts immediately after the first value-changing flat A000
store at `RES_0709:0412`. The prior
[first-EGA gate](RESIDENT_0709_064F_FIRST_EGA.md) proves the eight pinned
overlay callers reach this `0415` stop through original display, string
copy, printer, upcase, and glyph bytes, and compares C state through the
first change. This new C transition finishes that selected plane's
draw-segment store, the remaining plane and scanline passes, the drawer's
`RETF 0C` at `05E8`, and stops at printer `0709:0694`.

## Bounds, frame, and branches

The fixture rechecks the ignored `START.EXE` and `GAME.OVR` hashes in the
first-EGA test. The expanded drawer span `[0x726F,0x767B)` is 1,036 bytes,
SHA-256 `a9d1c72241b6810c3e0995b03cca98edc56cb76734e948333e360d82ef7e1840`;
it includes the full `RETF 0C` immediate. The first-EGA fixture also
pins the original SYSTEM upcase, Pascal copy, printer, resident display,
and eight overlay caller spans. Original media remain ignored under
`resources/`.

At `0415`, the drawer's BP frame has scanline 0 at `[BP-0F]`, selected
plane 0, 1, or 2 at `[BP-0C]`, glyph index `0x14` at `[BP-2]`, current
page byte offset at `[BP-4]`, one-cell count at `[BP-11]`, and color,
row, and column arguments above BP. The first `0412` page store has
already happened. The continuation at `0415..0438` reads the same font
byte and writes it to the draw segment from `DS:4A18`; later planes
write the font byte or zero to both page and draw segments according
to `DS:25C9+[3-plane] & color`. It processes scanlines 0 through 7,
adds 40 to `[BP-4]` after each scanline, restores mask `0F` at port
`03C5`, increments the drawer column argument, compares index 1 with
count 1, and returns to `0694`. The C model reproduces stack scratch
writes, flat memory, and the 14 CPU fields at the return boundary.

The independent saved-DOS
[clean-page glyph watch](SLOT_A_FIRST_CLEAN_PAGE_GLYPH_WRITE.md) reached
the real `0412` EGA writer on a different route. It corroborates the
instruction's native reach; the complete `0415..0694` comparison here
uses synthetic original-byte replay.

## Verification and limits

**48/48** eight-site × three-color × upper/lowercase original-byte/C
comparisons match every byte of 1 MiB flat RAM and all 14 CPU fields
at `0709:0694`. The imported first-EGA fixture also verifies its entry
and first-change C boundaries, and the original run continues to the
printer return `00BA:1858`. **12/12** direct `0415` boundary variants
seed nonzero later font scanlines and use both shared `A000` page/draw
segments and split `A000`/`B000` segments. They match full RAM and CPU
state at `0694`. AF is excluded from arithmetic flag comparison because
reached XOR instructions leave it undefined; the other meaningful
flags are compared. The C sources compile with
`-std=c11 -Wall -Wextra -Wpedantic -Werror`.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_0709_first_glyph_return.py
```

Unicorn maps video memory flat and does not emulate hardware EGA planes
or port side effects. This model accepts only the reached mode-one,
one-cell state after the first selected store; earlier drawer branches,
other modes, multiple cells, invalid arguments, and the printer's later
control flow remain outside this gate. It adds no strict full-C or
playable-game credit. Bounded
`deja "Poolrad RES_0709 0415 first glyph EGA remaining scanlines return 0694"`
timed out without a result, so no prior-session assertion was reused.
