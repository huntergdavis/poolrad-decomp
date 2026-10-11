# Resident field display callee at `RES_00BA:08EF`

Issue [#115](https://github.com/huntergdavis/poolrad-decomp/issues/115)
recovers the field-`+0x111` callee of the
[resident LOAD PIECES model](RESIDENT_LOAD_PIECES_0767.md). The portable
`pr_rec_res_00ba_08ef` is a **bounded local-control model**. It reproduces
the two field reads, distance calculation, branch, call order and arguments,
and returned AL. Its numeric formatter, Pascal string operations, private
literal, and display call remain explicit effect boundaries. Game versus
linked-library authorship is unresolved, and this adds no strict full-C or
game-identity credit.

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Loader mapping, bound, and callers

The hash-locked `resident_loader_map.unpack` maps `0x00BA:08EF` to loaded
image `0x0148F` and packed START.EXE file **`0x0156B`**. The latter was
listed as a source-file offset in both resident ledgers; it is not the next
loaded prologue. The function is loaded `0x0148F..0x01512`, 132 contiguous
copied bytes with zero fill, from file `0x0156B..0x015EE`. Its body SHA-256
is `c4d1ffe64d255ff80f1f812284a6145df3b56815fdf4a4aadff777798e32cb0d`.
Capstone 5.0.7 covers the body with 55 instructions. The only branch is
`0x014CB JBE 0x014F7`; both arms join before the single `RETF 8` at
`0x01510`. The next Pascal prologue is **loaded `0x01513`**, file
`0x015EF`, a separate function (`RES_00BA:0973`). The private Pascal
literal at loaded `0x0148D..0x0148E`, file `0x01569..0x0156A`, has
length one; its data byte is not published.

The frame is node far pointer offset/segment at `[BP+0xA]/[BP+0xC]`,
x byte at `[BP+8]`, and y byte at `[BP+6]`. `RETF 8` consumes all four
words. Direct caller evidence comprises one relocation-backed overlay far
call at unit `0x077:+0x0656` (GAME.OVR file `0x21690`) and two decoded
resident near calls at loaded `0x01415` and `0x0162F`. The first is the
reviewed `0767` field-111 call with `(node,0x20+class,row)`; the second
pushes the node, x `0x1A`, and a row-plus-one word. These are static
sites, not a claim that every route executes them.

## Local paths and callee graph

| Loaded address | Observed operation | C model |
| --- | --- | --- |
| `0x0149C..0x014B0` | Read byte node `+0x111`, compute 16-bit `abs(value - 0x3C)`, pass it and a stack-string pointer to local `0x01BD7`. | `read_byte`, then `FORMAT` event with distance `0..195`. |
| `0x014B3..0x014BD` | Call `0x0AF8:0B25` with length cap `0xFF` to copy the formatted Pascal string from first stack buffer to second. | `COPY_FIRST` event, cap `0xFF`. |
| `0x014C2..0x014CB` | Read node `+0x111` **again after** the first two calls. A value `<=0x3C` branches straight to display. | Second `read_byte`; choose path from its value. |
| `0x014CD..0x014F2` | On `>0x3C`, call `0x0AF8:0B0B` to copy the private one-byte Pascal literal into the first buffer, `0x0AF8:0B98` to append the numeric second buffer to that literal, then `0x0AF8:0B25` to copy the result into the second buffer with cap `0xFF`. | `COPY_LITERAL`, `APPEND_NUMBER`, and `COPY_FINAL` events in that order. |
| `0x014F7..0x01512` | Call `0x0709:0626` with low-byte arguments `(x,y,0x0A)` and second-buffer pointer, then `RETF 8` without changing AL. | `DISPLAY` event supplies returned AL. |

Thus the short path reaches three call sites and the longer path all six:
local `0x01BD7` once, runtime `0x0AF8:0B25` twice, `0x0AF8:0B0B` once,
`0x0AF8:0B98` once, and display `0x0709:0626` once. The local formatter
at loaded `0x01BD7` reads the **low byte** of the pushed distance,
zero-extends it, calls runtime `0x0AF8:1366`, and makes its own bounded
Pascal copy. It returns leaving the destination far pointer for this
routine's first `0B25` call. Direct inspection of the raw START.EXE
runtime entries confirms `0B25` caps the Pascal copy and `0B0B`/`0B98`
copy/append while retaining the destination pointer. Stack order at
`0B98` makes the private literal the destination and the formatted number
the appended source; the final `0B25` moves that result into the display
buffer. No private text or output string is recorded here.

The decoded body directly writes only stack locals. Other visible effects
belong to called routines, so the C model delegates all six call sites
instead of asserting complete formatter, string, or display parity. A
callback may change node `+0x111` before the second read. Only `io` and
output AL are required before execution; a missing or failed callback
returns `PR_REC_08EF_BOUNDARY` at its reached site, preserves prior callback
effects, and leaves output AL unchanged.

## Verification and prior work

Synthetic tests cover first-read distances for `0`, `0x3B`, `0x3C`,
`0x3D`, and `0xFF`; both branch outcomes after a callback changes the
field; six-call versus three-call order; copy caps and display arguments;
final AL; every callback failure position; and missing callbacks. The
focused test passes under `-Wall -Wextra -Wpedantic -Werror`. Full CTest,
public hygiene, and exact pushed head are recorded in the PR.

Before implementation, `deja "poolrad RES_00BA 08EF loaded 0x0148F field
0x111"` found no previous matching recovery beyond the issue assignment.
The [PR #110 note](RESIDENT_LOAD_PIECES_0767.md),
[resident callable ledger](resident_callable_ledger.csv),
[loader ledger](resident_loader_ledger.csv), and
[earlier Pascal-runtime note](RESIDENT_DAX_FILE_OPEN.md) provided leads.
All body bytes, bounds, calls, caller addresses, and raw runtime entry
instructions above were independently checked against the ignored DOS v1.3
binaries. No original bytes, private text, decoded media, or pixels enter Git.
