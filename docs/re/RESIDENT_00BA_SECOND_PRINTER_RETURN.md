# RES_00BA:1887 second-printer call and display return

This bounded gate starts after the flag-one display path's SYSTEM copy
returns at `00BA:1887`. It executes the original five-byte far call to
`RES_0709:0626`, stops at the printer's entry, and separately resumes at
the original printer return `00BA:188C` to execute the display epilogue
to its overlay caller. The printer body runs as original bytes in the
test; this C gate does not translate its internal effects.

## Original bounds and frame

The ignored `START.EXE` is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The complete expanded display body `[0x23B6,0x2432)` is 124 bytes,
SHA-256 `645caa0e81651dc9b2be1f156ea343d47f83a195f64f84a5c6fd828a89f50175`.
Its final eleven bytes, `00BA:1887..1891`, hash to
`bef4975c763f243ac454d60a3875242fccb942dc5646440e9f47e6d9a2fb2920`.
They decode as `CALL FAR 0709:0626`, `MOV SP,BP`, `POP BP`, and `RETF 0A`.

At `1887`, `SP=BP-14` and the five retained argument words are destination
offset `[BP-4]`, destination segment `SS`, color, row, and column. The
far call pushes return `188C:00BA`. The printer's own `RETF 0A` consumes
those five words and reaches `188C` with `SP=BP-4`. The display then
sets `SP=BP`, restores its saved BP, pops its overlay IP/CS, and removes
ten display-argument bytes. It does not change AL or flags. The C entry
report records the five printer words; it contains no private suffix text.

## Same-run differential evidence

The fixture starts at the two flag-one overlay sites and reaches the
first printer's `1858` return through original display/printer/glyph
bytes. It compares the existing C `1858` continuation at SYSTEM entry,
then the existing C SYSTEM adapter bridge at `1887`. It compares this
gate's C far call at `0709:0626`, executes the second printer's original
bytes to `188C`, and compares this gate's C epilogue at the overlay return.

The two sites × three colors × upper/lowercase glyph inputs give
**12/12** private-literal cases. Six further cases set the private
Pascal source length to zero at SYSTEM entry and give **6/6**
empty-string second-printer returns. At each C boundary, every byte of 1 MiB RAM and
all 14 CPU fields match original-byte execution (undefined AF is masked
after reached XOR instructions). The test also checks the return address,
five ordered printer arguments, printer cleanup SP, display cleanup SP,
and overlay return PC. C compilation uses strict C11 warnings.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_00ba_second_printer_return.py
```

This is call-frame and post-printer return parity for synthetic routes.
The complete second printer CPU/RAM effects are not modeled in C here;
saved-DOS natural flag reach, hardware EGA planes, full display effects,
strict full-C/game identity, and playable-game credit remain open.
Original media and native captures stay under ignored `resources/`.
Before implementation, bounded `deja "Poolrad RES_00BA 1887 second printer
0709 0626 return 188C flag one"` timed out without a result; no
prior-session assertion was reused. Existing repository evidence reused
and checked: [the 1887 SYSTEM copy gate](RESIDENT_00BA_1887_COPY_RETURN.md),
[first-printer call gate](RESIDENT_1816_FIRST_PRINTER_GATE.md), and
[unequal display control map](RESIDENT_UNEQUAL_DISPLAY_1816.md).
