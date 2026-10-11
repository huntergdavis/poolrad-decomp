# Unit 0x3bd entry 7 (+0x10C7, stub 0x3D5:0x0043): path-complete local-body model (issue #112)

`src/recovered/overlay_u3bd_wallset.c` models DOS v1.3 `GAME.OVR` unit
`0x3bd` public entry 7, the routine the merged LOAD FILES / LOAD PIECES
handler (unit 0x1c +0x0C74, opcode 0x21, PR #97's `load_wallset(ctx, id)`
boundary, FUNCTION_LEDGER.md function 23) calls on its wallset branch, and
the saved-game loader (unit 0x6f +0x1602) calls on replay. Classification:
**path-complete local-body model with callback boundaries**, not a full
translation (FUNCTION_LEDGER.md §1 vocabulary): the routine's own
instruction paths are reproduced; its sixteen far-call sites to eleven
distinct targets (seven runtime routines at eleven sites, four resident
routines at five sites) are four callbacks (string construction folded
into the loader callback's inputs). The strict full-C count is unchanged.
`ecl_helpers.c/h`, the COMPARE files and the resident files are untouched.

`deja "unit 0x3bd entry 7 +0x10C7 wallset opcode 0x21"` matched only this
worker's sessions. Reused with citation and re-checked from the bytes: the
handler decode of PR #97 (function 23) for the unit 0x1c site, the entry 6
note [U3BD_PIECES_ENTRY6.md](U3BD_PIECES_ENTRY6.md) for the shared loader
sequence and the PM's finding that `0x899:0x0000` is a DOS terminate, and
[ECL_BLOCK_LOAD.md](ECL_BLOCK_LOAD.md) §3 for the `RES_0802_0716` contract
(size 0 and nil pointer on failure).

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

Unit `0x3bd`: file offset 190,711, run-time segment `0x3D5`; entry 7 maps
to `+0x10C7` (stub `0x3D5:0x43`), file offset `0x2F9BE` (195,006).

| Item | Value |
| --- | --- |
| extent | `+0x10C7..+0x1205`, **319 bytes, 124 instructions**, one `retf 2`; the ledger bound of 319 is exact, no trailing data (entry 0's 5-byte stub follows at `+0x1206`) |
| span SHA-256 | `896cb77d4252e6948b83c9ef8f1888f68c5e01f11dde56bc6fa2719a61b1ac16` |
| frame | `sub sp, 0x110`; `[bp+6]` = id, read as a byte only (`mov al, [bp+6]`, twice) |
| branches | 2 (`+0x1129 je +0x1132`, `+0x1130 je +0x1160`), no loops, no indirect branches |
| call sites | 16 to 11 distinct targets: runtime `0xAF8` ×11 (`0x1366`, `0x0B0B` ×2, `0x0B98`, `0x0919`, `0x084A`, `0x1685` ×4, `0x0254`); resident ×5 (`0x802:0x0716`, `0x464:0x00FF` ×2, `0x899:0x0059`, `0x899:0x0000`); no overlay calls, no near calls |
| constants in code | `+0x109D` 3 characters and `+0x10A1` 4 characters (name parts), `+0x10A6` 32 characters (notice text); these are the 42 bytes between entry 6's `retf` and this entry |
| data-segment words | `[0x5376]` (file number), far pointer `[0x6A5C]` (1,024-byte destination), far pointer `[0x49D2]` (window 0) |

## Callers (two fixup-confirmed far calls; stub `0x3D5:0x43`)

| Site | Pushed id | Context |
| --- | --- | --- |
| unit 0x1c `+0x0CE1` (handler `+0x0C74`) | byte `[bp-3]` (its first operand value) with AH = 0 (`xor ah,ah` at +0x0CD2) | reached only when window-0 word 0xE6 (`[0x49D2]+0x1CC`) is non-zero; the handler itself writes window-0 word 0xC5 = v1 just before the call (+0x0CD4..+0x0CD8), and this routine writes the same word again at its end |
| unit 0x6f `+0x1BDE` (saved-game loader `+0x1602`) | low byte of window-0 word 0xC5 (`[0x49D2]+0x18A`) | after setting `[0x5376]` from window-1 byte 0x624 (+0x1BC1..+0x1BCA) and only when `[0x49FA] < 2` (`jae` skip); AH is whatever the preceding compare left, harmless since the callee reads a byte |

The unit 0x6f site shows the purpose of the word this routine writes: the
id of the last load is kept in window-0 word 0xC5 so a restored game can
replay the call with the same id.

## Instruction paths (local effects in order; `id` = `[bp+6]`)

| Offsets | Effect | Model |
| --- | --- | --- |
| `+0x10CE..+0x10E3` | `0xAF8:0x1366` with (`0:[0x5376]`, width 0, `[bp-0xA]`, 1): Str of the file number | `file_number` passed to `read_block` |
| `+0x10E8..+0x110D` | `0x0B0B` loads the 3-character literal into `[bp-0x10B]`, `0x0B98` appends the digits, `0x0B0B` loads the 4-character literal into `[bp-0x110]`: the two string arguments | name parts documented by length; the host builds the name |
| `+0x1112..+0x1120` | push id byte, `&[bp-2]` (size), `&[bp-6]` (pointer); `0x802:0x0716` (nine words with the two strings, `retf 0x12`) | `read_block(ctx, file_number, id, &block, &size, &capacity)` |
| `+0x1125..+0x1130` | `cmp size,0; je notice; cmp size,0x402; je copy`; fall into the notice | only a 1,026-byte block proceeds |
| `+0x1132..+0x1156` | `0x464:0x00FF`; `0x0919` with (`[0x8608]`, the 32-character literal, 0); `0x084A`; `0x899:0x0059` (result to `[bp-0xB]`, never read); `0x464:0x00FF` | `error_notice(ctx)` |
| `+0x115B` | `0x899:0x0000`: DOS terminate (`RES_0899_0000` → runtime `0xAF8:0x00E9` → `int 0x21` function 0x4C, PM decode of `START.EXE` loaded `0x08990..0x089C2` and `0x0B11E..0x0B123`). Does not return; the fall-through into `+0x1160` is dead on this path | `exit_program(ctx)`, then `EXITED` immediately |
| `+0x1160..+0x11DF` | `[bp-8] = 2`; four times: `0x1685` Move(block + `[bp-8]`, `[0x6A5C]` + q·0x100, 0x100); `[bp-8] += 0x100` | four `memcpy` of 256 bytes from block offsets 2, 0x102, 0x202, 0x302 into buffer offsets 0, 0x100, 0x200, 0x300 |
| `+0x11E4..+0x11ED` | `0x0254` FreeMem(block, size) | `release(ctx, block, size)` |
| `+0x11F2..+0x11FB` | `mov al,[bp+6]; xor ah,ah; les di,[0x49D2]; mov es:[di+0x18A],ax` | `window0_word_c5 = id` (zero-extended) |
| `+0x1200..+0x1203` | epilogue, `retf 2` | `OK` |

Side-effect order on the normal path: read, four copies, release, window
word. On the notice path: read, notice display and key read, program exit;
nothing else follows. The copies read block bytes 2..0x401, inside the
accepted size. The host bounds at the copy point are a missing block
pointer or a readable capacity below 0x402 (`BLOCK_BOUNDS`, nothing
copied) and a missing host buffer (`ARGUMENT`, checked there and not on
entry, because the original first touches `[0x6A5C]` after the read and
the size gate, so a bad-size block still notices and terminates with no
buffer at all; PM parity correction).

## Dependencies named

| Call-out | Identity | Modelled as |
| --- | --- | --- |
| `0xAF8:0x1366`, `0x0B0B`, `0x0B98` | runtime Str / string load / concat by argument shape | inputs of `read_block` (file number; literal lengths 3 and 4) |
| `0x802:0x0716` | `RES_0802_0716`, resident DAX block loader, `retf 0x12` (ECL_BLOCK_LOAD.md §3) | `read_block` |
| `0x464:0x00FF` (×2), `0xAF8:0x0919`, `0xAF8:0x084A`, `0x899:0x0059` | notice display and key read (GAMEPLAY_PATH.md names the key read) | `error_notice` |
| `0x899:0x0000` | `RES_0899_0000`, the exit routine; DOS terminate per the PM decode cited above | `exit_program`, required; `EXITED` returned at once |
| `0xAF8:0x1685` (×4) | Move by shape | `memcpy` |
| `0xAF8:0x0254` | FreeMem by shape | `release` |

## C contract and tests

`pr_u3bd_entry7(state, id, callouts, report)`; `state` holds the file
number, the 1,024-byte host buffer for `[0x6A5C]` and window-0 word 0xC5
(the merged loader state models window 0 as an array, so the PM can map
the word at index 0xC5 on integration). Each callback is required where
reached (`CALLOUT` with earlier effects done), as in PR #97 and #106.

`tests/recovered/test_overlay_u3bd_wallset.c`: six ids through the
success path (callback order read/release, loader arguments, the 1,024
bytes equal block bytes 2..0x401 in order, quarter boundaries, header
bytes and bytes past 0x402 never copied, the word stored zero-extended);
seven sizes other than 0x402 including 0, 0x401, 0x403, 0x400, 0x804 and
0xFFFF all take the terminal path (read, notice, exit; buffer and word
untouched, no release); each callback missing where reached; a short host
block (`BLOCK_BOUNDS`, nothing copied); a null buffer with a bad-size block
(read, notice, exit) and with a good block (`ARGUMENT` after the read, no
copy or release); a loader reporting 0x402 without a block (`BLOCK_BOUNDS`,
nothing copied); null contracts; optional report.

Measured: `cc -std=c11 -Wall -Wextra -Wpedantic -Werror` (gcc) builds the
library and test with no diagnostics and prints `unit 0x3bd entry 7 model
tests passed`; CMake build has no warnings; `ctest` passes 31 of 31
targets on main cb9c878, 31 of 31 on bb75729 and 32 of 32 on the rebased c22167f (the existing targets plus
`poolrad_overlay_u3bd_wallset_tests`).

## Limitations, stated plainly

- Not a full translation: the file-name construction, the DAX block
  loader, the notice display, the program exit and FreeMem are callbacks.
  "Wallset" is the handler's branch name in the merged ledger, not a claim
  about what the 1,024 bytes are.
- The literals' text is not reproduced; lengths only.
- A host that connects this model must treat `EXITED` as the program
  ending, not as a return to the handler.
