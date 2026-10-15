# RES_00BA copied-suffix post-glyph control and return

This bounded gate resumes the second printer at `RES_0709:0694` after
the copied suffix's first glyph drawer returns. It increments the byte
column, compares the one-based glyph index with the copied length, and
either enters the next drawer at `0709:01DF` or returns to the display
callee at `00BA:188C`. The C transition stops at those two boundaries;
it does not model either glyph drawer or later display effects.

## Dependency and original bounds

This branch is based on approved, published first-suffix-glyph head
`06530a4e518f6e11bac15009f18e10d740366846`, itself stacked on the
display dependencies through `cce4bd8`. It makes no rebased-integration,
shared CMake, or strict game-function count claim.

The ignored `START.EXE` is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The complete printer body `[0x76B6,0x7737)` is 129 bytes, SHA-256
`b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`.
The `0709:0694..06A6` control tail is 19 bytes, SHA-256
`a99211136bc3e92f64ad18457e57c760368383435486747c3a6a6bc6c850de6c`.
Its loop setup `0709:066D..0693` is 39 bytes, SHA-256
`39e75f859dc7aa6ce2df5f776a95cc6420c610d39f11386d7fa8e094d9d92f2b`.
The fixture also rechecks the prior display body, private source,
SYSTEM B0B/B25 helpers, glyph drawer, upcase helper, and eight overlay
prefixes by their existing pinned hashes. No original text or bytes are
published.

At `0694`, `INC BYTE [BP+0E]` advances the byte column and updates
arithmetic flags except CF. `MOV AL,[BP-102]; CMP AL,[BP-103]` selects
the edge. A mismatch branches to `066D`, increments the one-based glyph
index, pushes column, row, color, mode, copied character, and mode, then
uses `PUSH CS; CALL 01DF`. The first four word pushes preserve the
incoming AH; `XOR AH,AH` clears it before the character push. Equality
executes `MOV SP,BP; POP BP; RETF 0A` to `00BA:188C`.

## Differential result

The test reaches `0694` from two flag-one overlay sites through the
original first printer, SYSTEM suffix copy, second printer, and first
suffix glyph. It supplies one synthetic font byte at the first-glyph
boundary, as in the preceding gate. From that reached `0694` state,
**12/12** copied two-character cases match original-byte execution at
the second drawer entry `0709:01DF`. **12/12** direct variants set the
completed-count byte to the current index at `0694` and match the
printer return `00BA:188C`. **6/6** further continue variants set the
column byte to `FF` and match its wrap to zero. Each comparison covers
all 1 MiB RAM bytes and 14 CPU fields with defined flags; the test also
checks the eight-word drawer frame, preserved AH, index/column updates,
and printer return SP. C compilation uses strict C11 warnings.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_00ba_suffix_glyph_control.py
```

The completed-count and column-wrap cases are synthetic control
variants at the measured stop, not natural saved-DOS routes. The glyph
completion from `0415` to `0694` runs original bytes in this test; no C
parity is asserted for that interior. The second glyph, full printer,
hardware EGA planes, natural saved-DOS flag-one reach, strict full-C
identity, and playable-game credit remain open. Original media and
captures stay under ignored `resources/`. Bounded `deja "Poolrad
RES_00BA copied suffix first glyph 0709 0415 return 0694 second printer
control"` timed out without a result; no prior-session assertion was
reused. The preceding [first suffix glyph gate](RESIDENT_00BA_SUFFIX_FIRST_GLYPH.md)
and [first EGA change gate](RESIDENT_0709_064F_FIRST_EGA.md) were
rechecked as dependencies.
