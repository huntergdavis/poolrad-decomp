# RES_00BA copied-suffix printer return into two overlay parents

This gate follows the copied suffix's second glyph through its original
drawer completion to the printer loop at `0709:0694`. The C loop model
returns to display `00BA:188C`; a new C composition executes the existing
display epilogue, then each flag-one overlay parent's first five
instructions. It stops just after the parent's length-plus-four push,
before `077:+11DD` or `077:+2AC8`. The rest of either parent is outside
this gate.

## Sources and exact stops

The branch is stacked on the PM-approved, published second-glyph head
`4d891fd88601fcbfdf2baee2ba2cc065b511f7ca`. The ignored
`START.EXE` is 65,231 bytes (SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`);
the expanded display `[0x23B6,0x2432)` is 124 bytes (SHA-256
`645caa0e81651dc9b2be1f156ea343d47f83a195f64f84a5c6fd828a89f50175`).
Its final `00BA:1887..1891` eleven bytes hash to
`bef4975c763f243ac454d60a3875242fccb942dc5646440e9f47e6d9a2fb2920`.
The ignored `GAME.OVR` is 206,105 bytes (SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`).
The test rechecks the existing private suffix, printer, glyph drawer,
upcase helper, and all eight overlay-prefix hashes using the preceding
fixtures. It publishes no original source bytes or copied text.

| Parent | Original OVR window | Size | SHA-256 | Stop |
| --- | --- | ---: | --- | --- |
| `077:+11D1` | `[0x2220B,0x22217)` | 12 | `432394564917b2a0f34b170334812b65edaa9f35e871ae83a8dcf7e204b8f1c4` | `077:+11DD` |
| `077:+2ABB` | `[0x23AF5,0x23B02)` | 13 | `4320bd8f150aed52c5a3ce6156203bf146f7e23cde60100405b2e96eecfc9de4` | `077:+2AC8` |

Both parents execute `LES DI` on their node pointer, read its length
byte, clear AH, add four to AX, and push the resulting word. The first
pointer is `SS:[BP-3F]`; the second is `DS:[5D92]`. The C composition
preserves the distinct addressing modes, `LES` ES:DI result, arithmetic
flags, and stack write. Its input is the reached `00BA:188C` state,
whose frame holds the overlay return IP/CS.

## Differential result

For two flag-one overlay sites × three colors × two node-case inputs,
the test reaches the first printer, SYSTEM suffix copy, second printer,
first glyph, `0694` loop, and second glyph by original bytes. It supplies
one synthetic font byte for each copied glyph at the prior measured
boundary. The second glyph's first changed store is checked at
`A000:0143`; its remaining drawer runs as original bytes to `0694`.
The C loop return matches the original `00BA:188C` state. The C display
epilogue and parent instruction span then match original bytes in
**12/12** reached cases. Another **12/12** variants change only the
parent's node length after the printer return to `0F` or `FC`, exercising
the add-four flags and word push. Every comparison covers the full 1 MiB
RAM and 14 CPU fields with defined flags. Strict C11 compilation passes.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_00ba_suffix_parent_control.py
```

The late length variants are synthetic at `188C`; they are not natural
saved-DOS routes. The glyph interiors, complete second printer, later
overlay parent calls, hardware EGA planes, natural saved-DOS flag-one
reach, and strict full-C/game identity remain open. Original media and
captures stay under ignored `resources/`. Bounded `deja "Poolrad
RES_00BA 188C second printer return display callee parent control
1816"` timed out without a result; no prior-session assertion was
reused. The [second-printer return gate](RESIDENT_00BA_SECOND_PRINTER_RETURN.md),
[loop-control gate](RESIDENT_00BA_SUFFIX_GLYPH_CONTROL.md), and
[overlay caller-route map](RESIDENT_UNEQUAL_DISPLAY_1816.md) were
rechecked as dependencies.
