# RES_0709:0694 single-glyph printer exit to RES_00BA:1858

This bounded C transition starts when the first EGA glyph drawer has
returned to printer `0709:0694` for a one-character Pascal source. It
executes the printer's final loop check and `RETF 0A`, then stops at the
display callee continuation `00BA:1858`. The preceding
[first-EGA route](RESIDENT_0709_064F_FIRST_EGA.md) supplies the original
eight-caller, original-display, string-copy, printer, upcase, and glyph
context. The C model begins only at `0694`.

## Exact route and source

The ignored `START.EXE` is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The printer image `[0x76B6,0x7737)` is 129 bytes, SHA-256
`b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`.
The first-EGA fixture rechecks it, the full 1,036-byte glyph drawer,
SYSTEM upcase and Pascal copy helpers, resident display prefix, and the
ignored `GAME.OVR` hash before the route. No original bytes are published.

The reached tail `[0694,06A7)` has `INC BYTE [BP+0E]`,
`MOV AL,[BP-102]`, `CMP AL,[BP-103]`, `JNE 066D`, then at equality
`MOV SP,BP; POP BP; RETF 0A`. In this gate, one-based index and copied
count are both one. The increment changes only the low column byte;
`MOV AL` preserves AH from the glyph drawer; the compare supplies the
return flags. The far return pops the printer frame and ten argument
bytes, leaving `CS:IP=00BA:1858`. Other index/count states are rejected
by the bounded C entry.

## Differential result and limits

**48/48** eight-site × three-color × upper/lowercase one-glyph routes
match every byte of 1 MiB flat RAM and all 14 CPU fields at `1858`.
The imported first-EGA fixture compares its own C boundaries and then
lets original drawer and printer bytes run to the return in the same
execution. **6/6** direct `0694` variants set the byte column to `FF`
and AH to `5A`; the original and C returns agree on byte wrap to zero
and preservation of AH. AF is excluded from arithmetic flag comparison
where reached XOR instructions make it undefined; other meaningful
flags are checked. The C source compiles with
`-std=c11 -Wall -Wextra -Wpedantic -Werror`.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_0709_single_glyph_printer_exit.py
```

The saved-DOS [row-seven printer watch](SLOT_A_ROW7_TWELVE_CELL_PRINTER_PAGE.md)
independently reached a real `00BA:1858` printer return on another
route. It does not prove this synthetic one-glyph frame or this C
transition natively. Unicorn's flat A000 memory also does not emulate
EGA hardware planes. The display callee's flag-dependent continuation
after `1858`, the printer's next-glyph branch, and other source lengths
are separate gates. No strict full-C or playable-game credit follows.
Bounded
`deja "Poolrad RES_0709 0694 single glyph printer final return 00BA 1858"`
timed out without an indexed result; no prior-session assertion was
reused. Game media and captures stay under ignored `resources/`.
