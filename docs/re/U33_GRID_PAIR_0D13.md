# Unit 0x33 +0D13 grid pair

`pr_u33_grid_pair_0d13` translates the complete DOS v1.3 routine
`GAME.OVR` unit `0x33:+0D13..+0E9D` and composes its six near calls to
the merged unit-0x33 [byte-flags leaf](U33_BYTE_FLAGS_08A4.md) `+08A4`
and its four far calls to unit `0x9E` public entry 8 (`+0DE3`, the strict
[dice roll](U9E_ENTRY8_ROLL.md)) through the overlay stub `00B0:0048`.
It sweeps the 50 by 25 cell grid and writes a vertical pair of cells by
dice. The shared recipe (loaded-state stubs, whole-RAM/CPU differential,
synthetic DOSBox-X COM) is the one recorded in
[U33_GRID_ROLL_0000.md](U33_GRID_ROLL_0000.md); the recall attempt for the
cluster found nothing to reuse.

## Exact original, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x9E entries 8 and 9 | `+0DE3..+0E51`, 111 bytes, SHA-256 `9a2681e8b24a9a95838a11f2d7cbddea0c253d29e631db37d25fae017a14693b`; stub paragraph `00B0`, offset `48` |
| SYSTEM generator | START.EXE `0xBE36..0xBEBC` (135 bytes), as the merged [entry-8 roll evidence](U9E_ENTRY8_ROLL.md) pins it |
| Unit 0x33 `+08A4` leaf | `+08A4..+0B96`, the merged `pr_u33_byte_flags_08a4`, reached by `PUSH CS; CALL` with return IP site + 3 |
| Unit 0x33 | GAME file base `0xC5D1`; `+0D13` is an internal prologue, not a public entry |
| Function | GAME file `0xD2E4..0xD46E`; **395 bytes, 163 instructions, thirteen conditional branches, four far calls, six near calls** |
| Function SHA-256 | `6164aa24c865a4d56b05103552c143b8f37b8937cfa95d3f4fa7c227cd004925` |
| Far calls | `+0DD6` (1, `64h`), `+0DE9` (1, `0Ah`), `+0DFD` (1, 2) and `+0E27` (1, 3), each `LCALL 00B0:0048` with a fixup-backed segment word |
| Near calls | `+0D1E`, `+0D2C`, `+0D3A`, `+0D48`, `+0D56` and `+0E18` `CALL +08A4`, each preceded by `PUSH CS` |
| Exit | `RETF` at `+0E9D`; exact end `+0E9E` exclusive |

Every branch target lies inside the 395-byte body. Decoding every
unit-0x33 routine from its own prologue finds exactly one aligned
`PUSH CS; CALL +0D13`: `+121D` inside the `+11B4` driver, which also
calls `+0BDE`, `+0E9E` and `+0000` in that order and is itself reached
from the mode dispatcher at `+1244` (`+12DA`). The body reads the far grid
`DS:6716/6718` with the 50-byte row stride and `+7` cell byte and the
four-byte-stride DS table at `2747` exactly as `+0000` does. No gameplay
meaning is inferred for the written values; "grid pair" names the
vertical pair write.

## Complete ABI and ordered effects

The API enters after the far CALL with `SS:[SP]` return IP and `[SP+2]`
return CS and no arguments; `RETF` leaves SP four bytes above entry. The
routine pushes BP and reserves six local bytes: threshold `[BP-2]`, roll
value `[BP-3]`, column `[BP-4]` and row `[BP-5]`.

The threshold starts at 1; five `+08A4` calls then test AL bits `02h`
(threshold 1), `04h` (3), `40h` (4), `08h` (7) and `80h` (0), the last
set bit winning. For column 0..`31h` and row 1..`18h` the cell at
(column, row) and the cell above it at (column, row - 1) must both map
through `DS:2747 + 4 * cell` to `16h`. For such a pair a (1, `64h`) roll
above the threshold skips the cell. Otherwise a (1, `0Ah`) value above 8
becomes a (1, 2) roll plus 4, else (value + 1) shifted right once; a
sixth `+08A4` call with bit `40h` replaces it with a (1, 3) roll plus 3.
The cell receives value + `23h`; when the value is below 5 the cell above
receives value + `1Fh`. The row compare with `18h` and the column compare
with `31h` reread their live locals; the final flags are the column
compare's. AX, BX, CX, DX, SI, ES and DI return as the last address
computation, lookup, roll or leaf left them. Every input returns: the
loops are bounded by their byte counters.

Each far call is the loaded-state transfer of the merged dice chain
(`pr_u9e_entry8_fullstate_compose_0de3` on the same RAM and CPU words:
entry 8, its SYSTEM call at `0AF8:11C0`, the seed step at `0AF8:120F`
reading and writing the DS dword `4428`, and its `RETF 4`), entered only
when the five-byte site and the stub's far jump are the originals
(`CODE` otherwise), with `CALLEE` and `*chain_status` for a chain stop and
`TRANSFER` for a frame that does not return. Each near call pushes CS
and the return IP, enters the merged leaf at `+08A4` of the same segment
and requires it to return to that frame. Offsets wrap at 16 bits; words,
LES operands and return frames are physically contiguous; physical
addresses wrap at 1 MiB. Flag writes that a later instruction always
overwrites before any call or return (the address `MUL`/`ADD`s and the
`SHL`s feeding a `CMP`) are folded; every observable flag follows the
exact 8086 formulas. Null RAM/CPU or RAM that is not exactly 1 MiB is
rejected before mutation.

## Validation and parity review

The focused C11 test (`-Wall -Wextra -Wpedantic -Werror`, also run under
ASan/UBSan) finds a class byte whose merged-leaf result is independent of
stack residue and sets the threshold to 7, then predicts an all-eligible
grid cell by cell from the generator formula in the original's
column-major order, re-checking eligibility after each write because the
original rereads the grid; a grid with no eligible cell leaves the seed
and grid untouched; a stub that is not a far jump reports `CODE`; the
host guards leave the CPU words unchanged.

The original-byte oracle passes **147 returning cases**, each
comparing the entire 1 MiB RAM and all 14 CPU words with no flag mask:
72 random grids, tables (eligible density 0.3 to 1.0), class bytes,
window words, seeds, flags and ES; 52 class bytes 0..255 in steps of 5;
no eligible cell; an all-eligible default class; the grid across A20, at
offset wrap, under the stack and over the seed; the window under the
stack and across A20; eleven stack offsets at both segment ends on a zero
background; SS and DS across A20; a random background; and preserved DF.
All 163 instruction starts and all 26 conditional outcomes are reached, so no outcome is declared infeasible.

The DOSBox-X normal-core/386 synthetic oracle passes **70
cases** for this body in the shared cluster run, comparing all 14 CPU
words, the 128-byte frame window, the seed dword and the 1,257-byte grid
with no mask. The COM keeps the original unit 0x9E span at `CS:0DE3`,
copies the original SYSTEM bytes to the absolute paragraph `0AF8` at
start-up, holds the original `+08A4..+0B96` leaf and `+0D13..+11B3`
bodies at one base so their `PUSH CS; CALL` displacements stay valid, and
relocates the eleven far-call segment words to one stub paragraph by the
loader's arithmetic; the harness checks that its code, fixture table and
absolute SYSTEM copy stay clear of every DS global the bodies touch.
Maskable interrupts are disabled from frame priming through capture. The
result SHA-256 is `a7cafff592f0c7e3a41d4ca004112e22a7660fe5f6c155edd53b1305da894f33` with COM segment `0814`;
another DOS allocation changes the hash while retaining parity. Original
bytes, generated assembly/COM, results and logs stay under ignored
`resources/`; public fixtures are synthetic.

```sh
cmake -S . -B resources/build-grid-pair -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-grid-pair --target poolrad_overlay_u33_grid_pair_0d13_tests \
  poolrad_overlay_u33_grid_bands_0e9e_tests
ctest --test-dir resources/build-grid-pair \
  -R 'poolrad_overlay_u33_grid_(pair_0d13|bands_0e9e)_tests' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u33_grid_pair_0d13_audit.py --unicorn --native \
  --output resources/u33-0d13-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x33` `+0D13..+0E9D` with `+08A4` composed at six sites and entry 8 at four |
| `source_candidates` | `src/recovered/overlay_u33_grid_pair_0d13.c` |
| `signals` | Exact hash/CFG, one aligned caller inside the game-evidenced driver, the documented grid and table globals, whole-RAM/CPU differential with full instruction coverage and every roll-reachable branch outcome, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Every conditional outcome reached; loaded-state stubs only; natural saved-game reach unmeasured |

No classifier or count ledger is changed. Identity and strict credit
remain the PM's review decision.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_0033_0D13 --require-current-main
```
