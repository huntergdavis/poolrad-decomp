# Unit 0x3bd entry 2 (+0x047B, stub 0x3D5:0x002A): full translation (nibble lookup)

`src/recovered/overlay_u3bd_nibble.c` translates DOS v1.3 `GAME.OVR` unit
`0x3bd` public entry 2, the routine eight fixup-confirmed callers invoke
with the party x/y bytes `[0x6AAD]`/`[0x6AAE]` and the facing byte
`[0x6AAF]` to read one nibble of the first two 256-byte quarters of the
1,024-byte buffer at far `[0x6A5C]` that entry 7 fills
([U3BD_WALLSET_ENTRY7.md](U3BD_WALLSET_ENTRY7.md)). Classification:
**full translation** (FUNCTION_LEDGER.md §1 vocabulary): every instruction
path is reproduced with the same inputs and outputs, with one path whose
original output is an unwritten frame byte and is therefore flagged
rather than reproduced (below). The routine makes no far calls; its one
near call reaches the local leaf `+0x031F`, accounted for exactly and not
counted. `ecl_helpers.c/h`, the COMPARE files and the resident files are
untouched.

`deja "Poolrad unit 0x3bd entry 2 +0x047b"` matched only this worker's
entry 4 session (PR #124), which had already recorded the leaf's near
site `+0x048A` inside this entry. Reused with citation and re-checked
from the bytes: [U3BD_CELL_ENTRY4.md](U3BD_CELL_ENTRY4.md) for the leaf,
the map-id gate and the wrap (the same instruction sequence, re-decoded
here at its own offsets), the entry 7 note for the `[0x6A5C]` buffer,
[GAMEPLAY_PATH.md](GAMEPLAY_PATH.md) for the identities of `[0x84DC]`
(map id) and `[0x6AAD]`/`[0x6AAE]` (party x/y), and FUNCTION_LEDGER.md §1
for the store-word pseudo-address `0x35F` that writes `[0x6AAF]`.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

Unit `0x3bd`: file offset 190,711, code size 4,619, eight public entries,
run-time segment `0x3D5`; entry 2 maps to `+0x047B` (stub `0x3D5:0x2A`),
file offset `0x2ED72` (191,858). Decoded with objdump (`-m i8086`) and
Capstone 5.0.7 independently; both agree.

| Item | Value |
| --- | --- |
| extent | `+0x047B..+0x0579`, **255 bytes, 103 instructions**, one `retf 6`; the ledger bound of 255 is exact, no trailing data (entry 4 begins at `+0x057A`) |
| span SHA-256 | `92d75878d963a7f5766b1ea49e2f6c1fb51287775d06a5b6ca485fee8bc43658` |
| frame | `sub sp, 2`; `[bp+0xA]` = x (pushed first), `[bp+8]` = y, `[bp+6]` = selector; all read as bytes; x and y compared signed and extended with `cbw`; `[bp-1]` holds the result byte and is written only on five of the six exits |
| branches | 15, all forward (`+0x048F`, `+0x0496`, `+0x049D`, `+0x04A3`, `+0x04AA`, `+0x04B4`, `+0x04BE`, `+0x04C8`, `+0x04D3`, `+0x04FA`, `+0x04FE`, `+0x051E`, `+0x0522`, `+0x054B`, `+0x054F`), no loops, no indirect branches |
| call sites | 1: `+0x0489` `push cs; call +0x031F` (the local leaf, `retf 4`); no far calls |
| data-segment reads | byte `[0x84DC]` (map id) twice; far pointer `[0x6A5C]` at four sites (one per selector case); four ES reads (`es:[di]` twice, `es:[di+0x100]` twice) |
| writes | none outside the frame (the clamps rewrite the argument bytes `[bp+0xA]`/`[bp+8]` in place) |

The leaf `+0x031F..+0x034D` (49 bytes, 18 instructions, span SHA-256
`ab4d7f5e3d42b8d23dba77a563b5041f669850990444583890627278764f281b`) is
the one translated for entry 4; it is reproduced as a static helper in
this file so that no merged file changes, and no count is claimed for it.

## Instruction paths (local effects in order; `x` = `[bp+0xA]`, `y` = `[bp+8]`, `s` = `[bp+6]`)

| Offsets | Effect | Model |
| --- | --- | --- |
| `+0x0481..+0x048D` | push x byte, push y byte, `push cs; call +0x031F`; `or al,al` | `in_range = in_grid(x, y)` |
| `+0x048F` | `jne +0x04A6`: in range → the clamps (no-ops) and the selector chain | |
| `+0x0491..+0x049D` | `cmp byte [0x84DC],0; je +0x049F; cmp byte [0x84DC],10; jne +0x04A6` | out of range: map id 0 or 10 → zero; any other map id → wrap |
| `+0x049F..+0x04A3` | `[bp-1] = 0; jmp +0x0571` | `*result = 0`, `returned_zero`; neither `[0x6A5C]` nor the selector is looked at |
| `+0x04A6..+0x04B6` | `cmp x,15; jle; x = 0` then `cmp x,0; jge; x = 15` (signed) | `x > 15 → 0; x < 0 → 15` in that order |
| `+0x04BA..+0x04CA` | the same for y | `y > 15 → 0; y < 0 → 15` |
| `+0x04CE..+0x04D3` | `al = s; cmp al,0; jne +0x04FC` | selector chain |
| `+0x04D5..+0x04FA` | s = 0: `dx = cbw(x); ax = cbw(y) shl 4; les di,[0x6A5C]; di += ax+dx; al = es:[di]; and al,0xF0; xor ah,ah; shr ax,4; [bp-1] = al; jmp +0x0571` | `buffer[16·y + x] >> 4` |
| `+0x04FC..+0x051E` | s = 2: same address; `and al,0x0F` | `buffer[16·y + x] & 0x0F` |
| `+0x0520..+0x054B` | s = 4: `al = es:[di+0x100]; and 0xF0; shr 4` | `buffer[0x100 + 16·y + x] >> 4` |
| `+0x054D..+0x056E` | s = 6: `al = es:[di+0x100]; and 0x0F` | `buffer[0x100 + 16·y + x] & 0x0F` |
| `+0x054F` | s ∉ {0, 2, 4, 6}: `jne +0x0571` with `[bp-1]` never written | `UNSPECIFIED`, `*result` untouched |
| `+0x0571..+0x0577` | `al = [bp-1]`; epilogue; `retf 6` | `OK` |

Both coordinates are 0..15 at every read, so the offsets are
`0x000..0x0FF` (selectors 0 and 2) and `0x100..0x1FF` (selectors 4 and
6): the reads never leave the first two quarters of the 1,024-byte buffer.
The result is always 0..15 on a read path. A nil far pointer at `[0x6A5C]`
is reported as `ARGUMENT` at the read, not on entry, because the zero
path and the unspecified path never touch it (the parity rule of entries
4 and 7).

**The unwritten-result path.** The frame is `sub sp,2` with no store into
`[bp-1]` before the chain, so when no selector case matches, `+0x0571`
loads whatever byte that stack slot held before the call. The value is
not a function of the arguments or the globals and is not reproduced; the
model returns `PR_U3BD_NIBBLE_UNSPECIFIED` and leaves `*result` alone.
Whether this path is ever taken depends on the selector the callers pass,
which is bounded next.

## Callers (eight fixup-confirmed far calls; stub `0x3D5:0x2A`)

| Site | Routine | Pushed (x, y, s) | Context (decoded at the site) |
| --- | --- | --- | --- |
| unit 0x1c `+0x2DC7` | `+0x2CCF` (internal) | `[0x6AAD]`, `[0x6AAE]`, `[0x6AAF]` | only when a word is `0x4019` and `[0x49FA] == 1`; AL → `[0x6AB0]` |
| unit 0x25 `+0x04C9` | `+0x0482` (entry 6, stub `0x2B:0x3E`) | its own `[bp+0xA]`, `[bp+8]`, `[bp+6]` | inside a loop while a counter `[bp-3] < 2` and `[bp-2] == 0`; a non-zero result leaves the loop, zero increments the counter |
| unit 0x25 `+0x1A5B` | `+0x19DB` (entry 19) | `[0x6AAD]`, `[0x6AAE]`, `[0x6AAF]` | immediately after its entry 4 call (U3BD_CELL_ENTRY4.md); AL → `[0x6AB0]` |
| unit 0x33 `+0x0170` | `+0x012B` (nested, static link at `[bp+6]`) | its own `[bp+0xC]`, `[bp+0xA]`, `[bp+8]` | after it checks `[bp+0xC]`/`[bp+0xA]` in 0..15 itself and entry 3 (`0x3D5:0x2F`) returns non-zero; result zero → local 0, else 3 |
| unit 0x68 `+0x07A1` | `+0x073B` (internal) | `[0x6AAD]`, `[0x6AAE]`, `[0x6AAF]` | after the routine wraps `[0x6AAE]` itself (< 0 → 15, > 15 → 0); AL → `[0x6AB0]` |
| unit 0x68 `+0x0ADE`, `+0x0B25`, `+0x0B6A` | `+0x0989` (entry 2) | `[0x6AAD]`, `[0x6AAE]`, `[0x6AAF]` | after key 0x50 / 0x4B / 0x4D sets `[0x6AAF] = ([0x6AAF] + 4 / 6 / 2) mod 8` (`idiv` by 8, remainder); AL → `[0x6AB0]` |

Second level, for the two forwarding callers: unit 0x25 entry 6 has three
fixup-confirmed callers, all in unit 0x1c (`+0x03F5`, `+0x1803`,
`+0x1FF5`), each pushing `[0x6AAD]`, `[0x6AAE]`, `[0x6AAF]`. The nested
unit 0x33 routine is near-called twice from `+0x01AD`: at `+0x01D3` with
its caller's own three arguments, and at `+0x0209` with the selector
`[bp-4] = ([bp+6] + 4) mod 8` computed at `+0x01B3..+0x01C2` (the opposite
facing, parity preserved).

**Selector bound.** Every direct byte write to `[0x6AAF]` found in
`GAME.OVR` (scan of `mov [0x6AAF],al`, `mov byte [0x6AAF],imm`, `inc`,
`dec`, `add`, `sub` forms across all units) stores 0, 2, 4 or 6, or adds
2, 4 or 6 modulo 8: unit 0x25 `+0x0D9D`/`+0x0DA9`/`+0x0DB5`/`+0x0DC1`
(constants 0/2/4/6, the store-word class 4 region of FUNCTION_LEDGER.md),
unit 0x36 `+0x033D` (constant 6, the program body), unit 0x68
`+0x0ACF`/`+0x0B0D`/`+0x0B52` (the three turns) and `+0x0CCC`/`+0x0CD7`/
`+0x0CE2`/`+0x0CED` (constants 0/2/4/6). `START.EXE` contains none of
these forms for that address. So on every decoded path the selector is
even and in 0..6, and the unwritten-result path is not reached. A bulk
copy into the data segment (for example a saved-game restore) is outside
this scan and is the only way found for an odd value to arrive.

## C contract and tests

`pr_u3bd_entry2(state, x, y, selector, &result, report)`; `state` holds
the map id byte and the 1,024-byte host buffer for `[0x6A5C]`; the three
arguments are the raw pushed bytes; `result` receives AL on `OK`.
Statuses: `OK`, `ARGUMENT` (null state or result; null buffer at a read),
`UNSPECIFIED` (the unwritten-result path).

`tests/recovered/test_overlay_u3bd_nibble.c`: all 65,536 (x, y) byte
pairs for ten selectors (0, 2, 4, 6, 1, 3, 5, 7, 8, 0xFF) on four map ids
(0, 10, 1, 0x80) against an independent raw-byte restatement (0x00..0x0F
in range, 0x10..0x7F wrap to 0, 0x80..0xFF wrap to 15; out of range on
map 0 or 10 returns 0 before the selector is read; selector bit 2 picks
the quarter, bit 1 the low nibble), with the reported offset always below
0x200, the quarter and nibble matching the selector bits, and the wrap
applied before the unspecified exit; every selector 0..255 at one cell
with distinct nibbles pinned (0xA, 0x7, 0x3, 0xC; all others
`UNSPECIFIED` with the result untouched); every map id 0..255 at (16, 0)
(only 0 and 10 return 0, and they do so even with an odd selector); pinned
cells including (15, 15) → 0xFF / 0x1FF, (3, 2) → 0x23, (16, 3) → 0x130,
(0xFF, 3) → 0x3F, (0x80, 0x80) → 0x1FF, (0x7F, 0x10) → 0; the upper two
quarters inverted with no result changing; null state and result; a null
buffer on the zero and unspecified paths (never touched) and on the read
paths (`ARGUMENT`, result untouched); optional report.

Measured: `cc -std=c11 -Wall -Wextra -Wpedantic -Werror` (gcc) builds
the library and test with no diagnostics and prints `unit 0x3bd entry 2
translation tests passed`; the same with `-fsanitize=address,undefined`
passes; CMake build has no warnings; `ctest` passes 36 of 36 targets on
main 2efba50 (the existing targets plus
`poolrad_overlay_u3bd_nibble_tests`).

## Limitations, stated plainly

- The unwritten-result path is flagged, not reproduced; its value in the
  original is stack residue. The selector bound above rests on a byte-
  pattern scan of direct writers and the decoded call chains, not on a
  proof over bulk copies.
- The leaf `+0x031F` is duplicated as a static helper rather than shared
  with entry 4's file, to keep this change to new files.
- What the two nibbles per cell encode (quarter 0 and quarter 1 of the
  entry 7 buffer, selected by facing) is not established here; "nibble"
  is a positional name only. The callers are described as decoded, not
  modelled.
- The accounting documents are the integration lane's; this note claims
  one full translation for the PM to record, subject to the PM's view of
  the flagged path.
