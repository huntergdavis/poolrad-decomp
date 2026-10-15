# Copied-suffix overlay parents after SYSTEM B0B return

This gate starts only after original SYSTEM `0AF8:0B0B` executes its
`RETF 4` on the two unit-`077` parent frames. At `+11F2`, the first
parent immediately calls printer `0709:0626`. At `+2ADE`, the second
parent pushes a new local far pointer and calls SYSTEM concat
`0AF8:0B98`. The C transition stops at those callee entries; neither
callee body is part of this gate.

## Pinned original route and exact successors

This branch is stacked on PM-approved, published B0B head
`289af06adc65e1456e6202b81a37c7a38672b411`. Ignored `START.EXE`
is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
its B0B body `[0xB781,0xB79B)` is 26 bytes, SHA-256
`4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f`.
Ignored `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The preceding fixtures recheck the display, printer, glyph drawer,
private suffix, and eight overlay caller-prefix hashes.

| Reached parent PC | OVR file window | Bytes | SHA-256 | C stop |
| --- | --- | ---: | --- | --- |
| `077:+11F2` | `[0x2222C,0x22231)` | 5 | `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23` | `0709:0626` |
| `077:+2ADE` | `[0x23B18,0x23B22)` | 10 | `daedd70c0a61144728ecef03d9240f2c546f0a0fae1451c338417f078b51cde7` | `0AF8:0B98` |

The test privately loads the two hash-checked OVR Pascal sources from
the [B0B gate](RESIDENT_00BA_SUFFIX_PARENT_B0B.md), reaches the helper
entry from each flag-one overlay caller, and runs original helper bytes
to the actual parent return. The existing C B0B bridge independently
matches that return. `RETF 4` leaves the destination pointer at
`SS:SP`: `BP-47:SS` for `+11F2`, or `BP-12A:SS` for `+2ADE`. The test
checks that the original source's length and payload were copied into
that retained destination; no expected copy output is fabricated.

At `+11F2`, the five-byte far call pushes return `+11F7:CS` above the
retained printer arguments and enters `0709:0626`. At `+2ADE`, `LEA DI`
selects `BP-2A`; `PUSH SS; PUSH DI` adds that pointer above the retained
destination, then the far call pushes return `+2AE8:CS` and enters
`0AF8:0B98`. Both paths preserve arithmetic flags. The test checks
the ordered callee-entry stack words, SP, target CS:IP, and reported
pointers.

## Differential result and limits

Two flag-one overlay sites × three colors × two node-case inputs give
**12/12** original-byte/C comparisons after the naturally executed B0B
return and again at the immediate next call entry. Every comparison
covers all 1 MiB RAM bytes and 14 CPU fields with defined flags. The
focused C sources compile with strict C11 warnings.

```sh
python3 tests/recovered/check_resident_00ba_suffix_post_b0b_parent.py
```

“Naturally executed B0B return” describes the original helper execution
within this synthetic overlay route; no saved-DOS natural flag-one reach
is claimed. The subsequent printer and concat bodies, complete earlier
glyph/printer effects in C, hardware EGA planes, and strict full-C/game
identity remain open. Original media and captures stay under ignored
`resources/`. Bounded `deja "Poolrad U77 11F2 2ADE after SYSTEM B0B
parent printer B98 copy return frame"` timed out without a result; no
prior-session assertion was reused. The
[parent call-entry gate](RESIDENT_00BA_SUFFIX_PARENT_COPY_ENTRY.md),
[B0B step gate](RESIDENT_00BA_SUFFIX_PARENT_B0B.md), and
[caller-route map](RESIDENT_UNEQUAL_DISPLAY_1816.md) were rechecked as
dependencies.
