# Resident unequal-node display callee `RES_00BA:1816`

Issue [#123](https://github.com/huntergdavis/poolrad-decomp/issues/123) covers the
display callee used by the unequal-node branch of
[`RES_00BA:0767`](RESIDENT_LOAD_PIECES_0767.md). The public C entry
`pr_rec_res_00ba_1816` models its complete local branch and call order.
The printer and Pascal copy remain called-routine effects, so this is a
**bounded model**, with no strict full-C or game-function count claimed.

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Extent and frame

The hash-locked loader unpacker maps entry `00BA:1816` to expanded image
`0x023B6` and packed `START.EXE` `0x02492`. Its body ends at the only return,
`00BA:188F RETF 0x0A`, immediately before the next Pascal prologue at
`00BA:1892`. Thus its actual extent is expanded `[0x023B6,0x02432)` and
packed `[0x02492,0x0250E)`: 124 contiguous copied bytes, zero fill bytes,
SHA-256 `645caa0e81651dc9b2be1f156ea343d47f83a195f64f84a5c6fd828a89f50175`.
The loader ledger's `0x02492` endpoint is the *next* prologue, not the end of
this body. The private Pascal literal at `CS:1813` immediately precedes the
entry, has length two, and has length-plus-content SHA-256
`4bd4cdfadef19a0803e203a5c05b37a6a3e79e76d25454fa559743b912409844`.
Its bytes are not published.

The stack frame is flag word `[BP+6]`, row word `[BP+8]`, column word
`[BP+0x0A]`, node offset `[BP+0x0C]`, node segment `[BP+0x0E]`.
The body reads the low bytes of the first three words and the full far node
pointer; `RETF 0x0A` removes ten argument bytes. The `0767` unequal branch
at `00BA:0816..0828` pushes the node segment and offset, column, row, and a
zero flag, then calls `1816` through a near `PUSH CS; CALL` sequence.
The caller's equal branch prints directly through `0709:0626`.

## Decoded local behavior

| `00BA` offset | Original action | Public model |
| --- | --- | --- |
| `181C..1842` | Read node byte `+0x10D`. Zero selects color `0x0C`; otherwise read `+0x10E`, where exactly one selects `0x0E`, and every other value selects `0x0B`. | Ordered `read_byte` calls and exact three-way branch. |
| `1842..1858` | Push column, row, color, then node far pointer and call `0709:0626`. | `PRINT_NODE` effect. |
| `1858..185E` | Test only the low byte of the flag. Zero skips the rest. | Optional path. |
| `185E..1870` | Read node byte `+0` **after** the first printer returns, zero-extend it, add it to the zero-extended column, and push the resulting word. | Late read and byte column seen by the printer; unsigned byte wrap is tested. |
| `1870..188C` | Push row, color, and local stack-buffer pointer; call `0AF8:0B0B` to copy the private Pascal literal from `CS:1813`; then call `0709:0626` on that buffer. | Ordered `COPY_SUFFIX`, `PRINT_SUFFIX` effects. |
| `188C..1892` | Restore frame, `RETF 0x0A`. The epilogue does not write AL. | Return AL from the last successful printer. |

The body has three conditional branches: `JNE 1825→182D`,
`JNE 1836→183E`, and `JE 185C→188C`; both unconditional jumps converge
at `1842`. All targets remain in the body. The first printer can change
the later length byte in memory, which is why the model calls `read_byte`
again after that effect. The two printer calls consume the low bytes of the
scalar arguments as checked in the `0767` callee analysis. The original first
column push retains an unspecified AH from prior code, whereas the optional
second column is a zero-extended 16-bit sum; the public model expresses the
printer-visible low bytes.

On host callback failure, `PR_REC_1816_BOUNDARY` leaves output AL unchanged
and preserves prior effects. The missing callback is checked at its reached
call site. This is host behavior, not an original DOS error path.

## Eight overlay far callers

The relocation-backed inventory reports eight `GAME.OVR` far calls. Each
five-byte call is `CALL FAR 00BA:1816` with SHA-256
`dc9306d7e0ec9a98a78d80993496452e08743e42bb6fd8cd74c8e1d200e9e405`.

| Unit:offset | OVR file offset | Low-byte flag at call |
| --- | ---: | ---: |
| `05E:+0372` | `0x12B6D` | `0` |
| `06C:+2D4F` | `0x1C6CE` | `0` |
| `077:+0089` | `0x210C3` | `0` |
| `077:+0DE0` | `0x21E1A` | `0` |
| `077:+11CC` | `0x22206` | `1` |
| `077:+2AB6` | `0x23AF0` | `1` |
| `07F:+1B50` | `0x267D9` | `0` |
| `086:+33C3` | `0x2A3DB` | `0` |

These are static sites; the active LOAD PIECES route establishes the separate
resident near call. The site table does not prove each overlay route executes.

## Parity review and verification

| Item | Finding |
| --- | --- |
| `target_function` | `RES_00BA:1816`, packed source `[0x02492,0x0250E)` |
| `source_candidates` | `src/recovered/resident_unequal_display_1816.c`; no other candidate accepted by name alone |
| `signals` | Exact source provenance and SHA-256; prologue and sole return; three branch destinations; two printer calls and one copy call; frame argument order; eight matching overlay operands; live row-seven printer watch |
| `score` | Local control and callback order: 9/10; complete effect parity: unresolved |
| `verdict` | `match` for bounded local control; `unclear` for strict full-C and game-authored identity |
| `review_notes` | No implementation of `0709:0626` or `0AF8:0B0B` here. Full display memory effects, possible callee mutation, and original byte-for-byte literal output remain external. The ledger marks source ownership unresolved. |

Synthetic tests exercise all three color cases, both flag paths, column wrap,
late length mutation, returned AL from each possible final printer, every
callback failure position, and missing callbacks. The focused C test passes
with `-std=c11 -Wall -Wextra -Wpedantic -Werror`; CMake/CTest registration is
additive beside the existing `0767` target.

Before implementation, bounded `deja "RES_00BA:1816 unequal-node display
callee eight overlay callers"` was attempted. The sandbox could not lock the
local cache, an escalated 20-second retry timed out, and a narrower MCP recall
stalled. No past-session claim was reused. The merged
[`0767` analysis](RESIDENT_LOAD_PIECES_0767.md),
[`resident_loader_ledger.csv`](resident_loader_ledger.csv), and
[`row-seven native printer watch`](SLOT_A_ROW7_TWELVE_CELL_PRINTER_PAGE.md)
were used as leads and independently checked against the ignored original
inputs. Only addresses, counts, and hashes are published.
