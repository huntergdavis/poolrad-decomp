# Unit 0x7d entry 1 (+0x03D5, stub 0x8C:0x25): bounded model with the +0x02F2 carry normalizer

`src/recovered/overlay_u7d_entry1.c` models DOS v1.3 `GAME.OVR` unit `0x7d`
public entry 1, the call the unit `0x68` coordinate writer makes last
([U68_COORDINATE_WRITER_073B.md](U68_COORDINATE_WRITER_073B.md)), and its
near callee `+0x02F2`. Classification: **bounded path-complete model, not
strict full C**. Its other near callee `+0x0000` (754 bytes) is a host hook;
the limit table and the far-pointer targets are host state. **No
game-function count or credit is claimed**; the PM decides after review.

`deja "Poolrad unit 0x7D entry 1 +0x03D5 0x8C:0x25"` found no prior session.
Reused with citation: the coordinate-writer note (caller, args `(1, 2)` /
`(1, 1)`, stub mapping `0x8C` = unit `0x7d`), the ledger rows
`0x7d:0x03d5` / `0x02f2` / `0x0000` of `overlay_function_ledger.csv`, and
GAMEPLAY_PATH.md's window description (window 0 = far `[0x49D2]`).

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

Unit `0x7d`: code at file offset `146,892` (`0x23DCC`), stub paragraph `0x8C`.
Decoded with Capstone (16-bit).

| Item | Value |
| --- | --- |
| entry 1 extent | `+0x03D5..+0x0479` = file `0x241A1..0x24245`, **165 bytes, 67 instructions**, `retf 4` at `+0x0477`; next prologue `+0x047A`, so the ledger bound is exact and there is **no trailing data**. SHA-256 `7bd8c6d35073b0d7101c4a3fbe2154e916a1f72424636636fcf0b48718f148a4` |
| frame | `sub sp, 0x12`: `[bp-0xE..bp-1]` = seven-word array, `[bp-0xF]` byte i, `[bp-0x10]` byte k, `[bp-0x11]` byte n; args `[bp+8]` n, `[bp+6]` idx (each pushed as a word, read as a byte) |
| calls | near `+0x02F2` (in the loop, far-style frame: `push ss; push di` = far pointer to the local array, then `push cs; call`), near `+0x0000` (final) |
| near callee `+0x02F2` | `+0x02F2..+0x03A2`, **177 bytes** (SHA-256 `8f39276ad357b6e05367ffc080afedc0df5a5cca2f3cbf54b3d96db18bea3dd9`), `retf 4`, one far pointer argument at `[bp+6]`, no calls |
| near callee `+0x0000` | `+0x0000..+0x02F1`, 754 bytes, `retf 4`, 248 instructions per the ledger; calls runtime `0xAF8:0x16A8` (fill, argument shape) and unit `0x9e` entry 2 (stub `0xB0:0x2A`, an overlay routine, not resident; corrected in U7D_ZERO_CALLEE.md). It is translated in `overlay_u7d_zero.c` as a bounded model |
| far callers of `0x8C:0x25` | 8 sites across `GAME.OVR`; the pushed (n, idx) pairs: `(1, 1)` x2, `(1, 2)` x3, `(12, 3)` x2, `(local byte, 1)` x1. All idx values are within 0..3 |

## Instruction paths (entry 1)

| Offsets | Effect |
| --- | --- |
| `+0x03DB..+0x0407` | for i = 0..6: `w[i] = es:[[0x49D2] + ((0x49C6 + i) * 2 + 0x6E00)]`. The displacement sum is `0x1018C + 2i`, i.e. `+0x018C + 2i` modulo 64K: **window 0 words `+0x018C..+0x0199`** (the low word of the far pointer is taken as 0, as for the other window-0 accessors; the model rejects a window shorter than `0x19A`) |
| `+0x0409..+0x0414` | `n = [bp+8]`; `al = 1; cmp al, n; ja +0x043A`: for n = 0 the loop is skipped |
| `+0x0416..+0x0438` | for k = 1..n (n iterations, byte counter): `inc word [bp + 2*idx - 0xE]` (16-bit), then near `+0x02F2` on the local array |
| `+0x043A..+0x0466` | write all seven words back to the same window-0 addresses (unconditional) |
| `+0x0468..+0x0471` | push word (AH:n), push word (AH:idx), near `call +0x0000`. **AH is stale `0x93`**: the write-back loop's last instruction touching AX is `shl ax, 1` on `(0x49C6 + 6) * 2 = 0x9398`, and the pushes only load AL. The callee therefore receives `0x93nn`, `0x93mm`; callees that read bytes do not see it |
| `+0x0474..+0x0477` | `mov sp, bp; pop bp; retf 4` |

`idx` is used unchecked as a word index into the 14-byte local array: values
above 6 would write the saved BP, the return address and the caller's frame.
No call site uses one; the model returns `PR_U7D_ARGUMENT` for `idx > 6`.

## Normalizer `+0x02F2` (carry across seven words)

For i = 0..6 in order, with `limit[i]` the unsigned word at DS:`0x363A + 2i`:

* `w[i] < limit[i]` (unsigned, `jb`): nothing.
* otherwise, `i < 6`: `w[i+1] += 1`, `w[i] -= limit[i]` (16-bit); the loop
  then reaches `i+1`, so a carry can ripple in one pass.
* `i == 6`: **walk the linked list** whose head is the far pointer at DS
  `[0x5D96]` (null = both words zero): for each node, `inc word node+0x30`,
  then `node = far pointer at node+0x104`. `w[6]` is not reduced.

Nothing else is written; the array is the caller's stack copy.

**The limit table is host state.** The image bytes at DS:`0x363A` in
`START.EXE` (paragraph `0xC7C`) are zero, so the live values are filled at
run time or live elsewhere; they are not established here. The list nodes'
meaning (what `+0x30` counts) is not asserted.

## What this means for the coordinate writer and the minute question

Window 0 words `+0x018C + 2*idx` are the words this entry updates. The
coordinate writer calls it with `idx = 1` (word `+0x018E`, bit 0 of window-1
word `0x594` clear) or `idx = 2` (word `+0x0190`, bit set), `n = 1`. So the
**bytes at window 0 `+0x18E`/`+0x190` that VP's trace asks about are written
by this routine's write-back** (and by the carry into the next word), not by
the unit `0x68` span, as the earlier note's "no minute write in the span"
already left open. Whether those words are minutes, and what the other
`0x8C:0x25` sites add, is p5's question and is not claimed. Window 0 words
`+0x018C..+0x0199` are the full set rewritten.

## Verification

`tests/recovered/test_overlay_u7d_entry1.c` fabricates window 0, two limit
tables (one with limits 0..6 to force a carry at every word), a three-node
list with one counter at 0xFFFF, and a hook recorder. It compares the model
with an independent restatement over every n 0..6 (plus 250..255) and idx
0..6 for five starting word sets including 0xFFFF everywhere, and checks:
exactly the seven words change, the node counters (wrapping), the hook
words `0x93nn`/`0x93mm`, that the hook sees the written-back words, that w[6]
is never reduced, the null-list case, and that every ARGUMENT guard (idx 7,
short window, short/null/cyclic node) leaves state untouched. The
restatement shares my reading of the bytes; the byte-level correspondence is
the decode in the table above, not a DOS replay.
