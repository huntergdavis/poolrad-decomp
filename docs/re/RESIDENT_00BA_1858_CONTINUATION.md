# RES_00BA:1858 display continuation after the first printer

This bounded gate resumes the unequal-node display callee after its
first `RES_0709:0626` printer returns to `RES_00BA:1858`. It follows the
low-byte flag split. Flag zero executes the callee's `RETF 0A` to the
overlay caller. Flag nonzero builds the optional suffix-copy frame and
stops at SYSTEM `0AF8:0B0B`, before that helper executes. The original
route begins at each of the eight pinned overlay caller prefixes and
reaches `1858` through original display, printer, upcase, and glyph
bytes. The C comparison starts at `1858`.

## Original bounds and frame

The ignored `START.EXE` is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Its expanded `RES_00BA:1816` body `[0x23B6,0x2432)` is 124 bytes,
SHA-256 `645caa0e81651dc9b2be1f156ea343d47f83a195f64f84a5c6fd828a89f50175`.
The test pins that complete body and the existing fixture rechecks the
printer, glyph drawer, upcase and string-copy helpers, resident prefix,
eight overlay prefixes, and `GAME.OVR`. The private Pascal literal at
`CS:1813` is not read before the nonzero stop and is not published.

At `1858`, the callee's BP frame holds flag `[BP+6]`, row `[BP+8]`,
column `[BP+0A]`, and node offset/segment `[BP+0C/+0E]`. `CMP BYTE
[BP+6],0` reads only the flag low byte. If zero, `MOV SP,BP; POP BP;
RETF 0A` leaves the overlay return address in CS:IP and consumes ten
argument bytes. If nonzero, `LES` reads the node pointer **after** the
first printer's effects, reads node byte `+0` as a length, zero-extends
both it and the original column low byte, adds the two words, and pushes
the resulting column. It then pushes row, selected color, destination
`SS:[BP-4]`, source `CS:1813`, and the far-call return `00BA:1887`.
The C transition stops with `CS:IP=0AF8:0B0B`; no suffix copy or second
printer effect is modeled.

## Differential and native evidence

**36/36** zero-flag routes match original-byte execution at their overlay
returns. **12/12** nonzero-flag routes match at SYSTEM copy entry. These
are eight overlay sites × three colors × upper/lowercase glyph input;
the two flag-one sites account for 12 nonzero cases. **6/6** direct
`1858` variants change the node length after printer return to 2 and
the column low byte to `FF`, checking the late read and pushed word
`0101`. Every case compares all 1 MiB flat RAM bytes and all 14 CPU
fields at its stop; the existing comparison masks undefined AF after
reached XOR instructions. The C sources compile with
`-std=c11 -Wall -Wextra -Wpedantic -Werror`.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_00ba_1858_continuation.py
```

The saved-DOS [row-seven twelve-cell watch](SLOT_A_ROW7_TWELVE_CELL_PRINTER_PAGE.md)
independently reached loaded `08DE:1858` after the native printer and
later reached parent `08DE:082B` on a different route. That public watch
pins its page and addresses; it does not supply this synthetic frame's
full CPU/RAM parity or establish which `1858` flag branch ran. The
nonzero path's copy and second printer remain dependencies. Flat A000
replay does not emulate hardware EGA planes. No strict full-C,
game-identity, or playable-game credit is claimed.

Bounded `deja "Poolrad RES_00BA 1858 display callee after printer return
overlay route"` timed out without a result; no prior-session assertion
was reused. Original media and any native captures stay under ignored
`resources/`. This public note contains only addresses, hashes, counts,
and synthetic values.
