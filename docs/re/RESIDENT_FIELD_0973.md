# Resident field `+0x11B` display callee at `RES_00BA:0973`

Issue [#122](https://github.com/huntergdavis/poolrad-decomp/issues/122)
recovers the `+0x11B` callee of the merged
[resident LOAD PIECES model](RESIDENT_LOAD_PIECES_0767.md). The portable
`pr_rec_res_00ba_0973` is a **bounded local-control model**: it follows
all decoded reads, branches, call arguments, and returned AL, while the
numeric formatter and display routine remain explicit callback boundaries.
The raw node fields and style bytes are not assigned visual meanings.
Game versus linked-library authorship remains unresolved; this adds no
strict full-C or game-identity count.

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Proven span and callers

The hash-locked `resident_loader_map.unpack` maps DOS `0x00BA:0973` to
loaded image `0x01513` and packed START.EXE file `0x015EF`. The body is
loaded `0x01513..0x01568`, **86 contiguous copied bytes** from file
`0x015EF..0x01644`, with zero fill bytes. Body SHA-256:
`a5093db76117998bf62e1d6a3b0ca1816f6109dd7d3bca570070a09c0f73ebd0`.
Capstone 5.0.7 covers the span with **32 instructions**, three internal
branches (`JAE`, `JMP`, `JE`), two calls, and one `RETF 0x0A` at
`0x01566`. No branch leaves the span. The next Pascal prologue is loaded
`0x01583` (`RES_00BA:09E3`); the 26 bytes after the return and before
that prologue are not included in this function.

The frame holds node far pointer offset/segment at `[BP+0xC]/[BP+0xE]`,
x byte at `[BP+0xA]`, y byte at `[BP+8]`, and an override flag byte
at `[BP+6]`. `RETF 0x0A` consumes the five words. Static direct caller
evidence comprises two relocation-backed overlay far sites and two decoded
resident near sites:

| Caller | Site | Argument evidence |
| --- | --- | --- |
| Overlay unit `0x06C` | `+0x2065`, GAME.OVR file `0x1B9E4` | Active far pointer `[0x5D92]`, x `4`, y `0x12`, flag from a caller frame byte. |
| Overlay unit `0x077` | `+0x068F`, GAME.OVR file `0x216C9` | Caller-local far pointer, x `4`, y from caller row plus one, flag `0`. |
| Resident LOAD PIECES `0x00BA:0767` | Loaded `0x01457` | Node far pointer, x `0x24+class`, current row y, flag `0`. |
| Resident `0x00BA:09E3` | Loaded `0x015F6` | Caller far pointer, x `0x21`, row-plus-one y, flag `0`. |

The overlay sites are fixup confirmed; the resident sites are from the
candidate-seeded decoded near-call walk. These addresses establish static
reachability, not that every caller runs on the same game path.

## Local paths and effect boundaries

| Loaded address | Decoded action | Portable model |
| --- | --- | --- |
| `0x0151A..0x01531` | Read byte node `+0x11B`, then byte node `+0x32`. An unsigned first byte **below** the second selects style `0x0E`; equal or above selects `0x0A`. | Two ordered `read_byte` callbacks and unsigned comparison. |
| `0x01535..0x0153F` | If low byte of `[BP+6]` is nonzero, replace the selected style with `0x0D`. | Override from the explicit input flag. |
| `0x0153F..0x0155B` | Push low-byte x, y and style, then a stack Pascal-string destination; read node `+0x11B` **again** and pass its low byte to local formatter `0x01BD7`. | Third `read_byte`, then `FORMAT_NUMBER` event with that fresh byte. |
| `0x0155E..0x01568` | Call `0x0709:0626` with that string and low-byte `(x,y,style)`; return without modifying AL. | `DISPLAY` event supplies returned AL. |

The formatter at loaded `0x01BD7` reads its low-byte numeric argument,
zero-extends it, calls the separate runtime number formatter at
`0x0AF8:1366`, performs a bounded Pascal copy, and returns leaving the
destination far pointer for `0x0709:0626`. Its full numeric formatting
and the display side effects are beyond this model. The 0973 body itself
writes stack locals only; it does not directly write game globals. Each
callback is checked when reached. On a missing or failed callback, the
model stops with `PR_REC_0973_BOUNDARY`, leaves output AL unchanged, and
retains earlier callback effects. Only `io` and output AL are required
before the first read.

The display callee starts at loaded `0x076B6`. Its entry reads the string
pointer at `[BP+6]`, bounds x and y from low bytes `[BP+0xE]` and
`[BP+0xC]`, then reads low-byte x, y, and style from `[BP+0xE]`,
`[BP+0xC]`, and `[BP+0xA]`; its local return is `RETF 0x0A`. This confirms
the five-word argument stack passed by 0973. Formatter and display behavior
remain callback effects; their own full paths are not claimed here.

## Verification and prior work

Synthetic tests cover unsigned less/equal/greater comparisons including
zero and maximum byte edges, nonzero override, distinct first and third
field values, the exact three-read/two-effect order, display arguments,
final AL, every callback failure position, and missing host callbacks.
The focused test passes with `-Wall -Wextra -Wpedantic -Werror`; full
CTest, hygiene, and exact pushed head are recorded in the PR.

Before implementation, `deja "poolrad RES_00BA_0973 loaded 0x01513 field
0x11B"` found no matching prior implementation. The merged
[PR #110 note](RESIDENT_LOAD_PIECES_0767.md),
[loader ledger](resident_loader_ledger.csv), and
[callable ledger](resident_callable_ledger.csv) supplied entry and caller
leads. Those leads, the body bytes, the caller ABI, and the formatter
instruction path were independently checked against the ignored DOS v1.3
binaries. The shared CMake target name was coordinated in
[issue #122](https://github.com/huntergdavis/poolrad-decomp/issues/122#issuecomment-6111354763).
No executable bytes, private strings, decoded media, or pixels enter Git.

This branch also reuses the earlier isolated local draft commit
`8c6daf44b1133b9258ee5dc4a565835b5dec4c6c` for its C API and
synthetic cases. A fresh `deja "RES_00BA_0973"` search surfaced only the
resident ledger lead. The copied-byte hash, complete local disassembly,
both overlay fixups, two resident near calls, and both callee argument
contracts were rechecked independently before reusing that draft.
