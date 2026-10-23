# Unit 0x33 +0BDE grid diagonal

`pr_u33_grid_diagonal_0bde` translates the complete DOS v1.3 routine
`GAME.OVR` unit `0x33:+0BDE..+0D12` together with its private pair writer
`+0B97..+0BDD`, composes its two near calls to the merged unit-0x33
[byte-flags leaf](U33_BYTE_FLAGS_08A4.md) `+08A4` and its four far calls to
unit `0x9E` public entry 8 (`+0DE3`, the strict
[dice roll](U9E_ENTRY8_ROLL.md)) through the overlay stub `00B0:0048`. It
draws a diagonal of cell pairs across the 50 by 25 grid and places marked
pairs on it by dice. The shared recipe is the one recorded in
[U33_GRID_ROLL_0000.md](U33_GRID_ROLL_0000.md) and
[U33_GRID_PAIR_0D13.md](U33_GRID_PAIR_0D13.md); the recall attempt for the
cluster found nothing to reuse.

## Exact original, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x9E entries 8 and 9 | `+0DE3..+0E51`, 111 bytes, SHA-256 `9a2681e8b24a9a95838a11f2d7cbddea0c253d29e631db37d25fae017a14693b`; stub paragraph `00B0`, offset `48` |
| SYSTEM generator | START.EXE `0xBE36..0xBEBC` (135 bytes), as the merged [entry-8 roll evidence](U9E_ENTRY8_ROLL.md) pins it |
| Unit 0x33 `+08A4` leaf | `+08A4..+0B96`, the merged `pr_u33_byte_flags_08a4`, reached by `PUSH CS; CALL` with return IP site + 3 |
| Unit 0x33 `+0B97` pair writer | GAME file `0xD168..0xD1AE`; **71 bytes, 29 instructions, no branch, `RETF 6`**; SHA-256 `75c464f3df7839fc6a44ab167138b0addb4a329758a775b3255c50a35108b52a`; its only callers are this body's three sites |
| Unit 0x33 | GAME file base `0xC5D1`; `+0BDE` is an internal prologue, not a public entry |
| Function | GAME file `0xD1AF..0xD2E3`; **309 bytes, 138 instructions, nine conditional branches, four far calls, five near calls** |
| Function SHA-256 | `38483d20e747e2ee60318c1d09f5990e1674285fb7415851240e2cb111ffaaa2` |
| Far calls | `+0C0E` (1, `64h`), `+0C21` (5, 4), `+0C9C` (1, `14h`) and `+0CCE` (1, 9), each `LCALL 00B0:0048` with a fixup-backed segment word |
| Near calls | `+0BED` and `+0BFB` `CALL +08A4`; `+0CB6`, `+0CFB` and `+0D0C` `CALL +0B97`; each preceded by `PUSH CS` |
| Exit | `RETF` at `+0D12`; exact end `+0D13` exclusive |

Every branch target lies inside the 309-byte body. Decoding every
unit-0x33 routine from its own prologue finds exactly one aligned
`PUSH CS; CALL +0BDE`: `+1219`, the first generator call of the
[`+11B4` driver](U33_GRID_DRIVER_11B4.md). The pair writer `+0B97` is
reached from no other routine. The body reads the far grid `DS:6716/6718`
with the 50-byte row stride and `+7` cell byte like the other unit-0x33
generators; it does not read the `2747` table. No gameplay meaning is
inferred for the written values `32h`..`35h`; "grid diagonal" names the
shape.

## Complete ABI and ordered effects

The API enters after the far CALL with `SS:[SP]` return IP and `[SP+2]`
return CS and no arguments; `RETF` leaves SP four bytes above entry. The
routine pushes BP and reserves six local bytes: threshold `[BP-1]`, pair
count `[BP-2]`, saved column `[BP-3]`, column `[BP-4]` and row `[BP-5]`;
the threshold and count start at 0.

Two `+08A4` calls test AL bits `20h` (threshold `23h`) and `10h`
(threshold `4Bh`). A (1, `64h`) roll above the threshold returns at once.
Otherwise the column is `22h` minus a (5, 4) roll, lowered by one while
the signed remainder of (column + 2) `IDIV` 7 is positive, and saved. For
row 0..`18h`, a column at or below `31h` (signed) writes `32h` at
(column, row) and `33h` at (column + 1, row), where a cell is grid +
`32h` * row + column + 7; then a (1, `14h`) roll of 1, or an odd pair
count (`SHR AL,1` carry), calls the pair writer, and the column advances.
A nonzero count returns. Otherwise the row becomes `0Ch` minus a (1, 9)
roll plus 4, the column the saved column plus that row, and the pair
writer runs for (column, row) and (column + 1, row + 1).

The pair writer takes the column and row as sign-extended byte arguments
and the caller's BP as a static link: it writes `34h` at (column, row) and
`35h` at (column + 1, row), increments the caller's `[BP-2]` through the
link and returns with `RETF 6`. The model folds it as a static function
that pushes CS, the return IP (site + 3) and BP, performs the same reads,
`MUL`s and writes, pops BP and discards the six argument bytes, so every
frame byte below SP matches the original; when the separately recovered
`+0B97` model merges, this fold is to be replaced by a composition of it.

The final flags are the gate compare's on the early return, the count
compare's when a pair was placed in the loop, or the last `INC` of the
fallback pair writer. AX, BX, CX, DX, SI, ES and DI return as the last
address computation, roll, leaf or pair write left them. Every input
returns: the adjust loop lowers a byte that is at least `0Eh` by at most
six steps, and the row loop is bounded by its byte counter.

The far calls, the loaded-state stub checks, the `CODE`, `CALLEE`,
`TRANSFER` and `ARGUMENT` statuses, the near-call frames, the wrap rules
and the folding of flag writes that a later instruction always overwrites
are exactly as documented for [`+0D13`](U33_GRID_PAIR_0D13.md).

## Validation and parity review

The focused C11 test (`-Wall -Wextra -Wpedantic -Werror`, also run under
ASan/UBSan) finds a residue-independent class byte whose leaf result sets
bit `10h`, predicts the whole grid, the seed and the pair count from the
generator formula for a seed that passes the gate (diagonal, pair
placement and the fallback pairs), checks a seed whose first roll exceeds
the threshold returns after one roll with the grid untouched, a stub that
is not a far jump reports `CODE`, and the host guards leave the CPU words
unchanged.

The original-byte oracle passes **122 returning cases**, each
comparing the entire 1 MiB RAM and all 14 CPU words with no flag mask:
60 random grids, tables, class bytes, window words and bytes, modes,
class tables, seeds, flags and ES; 37 class bytes 0..255 in steps of 7;
no eligible cell; the grid across A20, at offset wrap, under the stack
and over the seed; the window under the stack and over the class byte;
eleven stack offsets at both segment ends on a zero background; SS and DS
across A20; a random background; preserved DF; and three directed seeds
whose 5d4 of 5..8 after a passing gate start the diagonal at column
`1Ah` so the row loop reaches its column limit. All 138 instruction starts and all 18 conditional outcomes are reached, so no outcome is declared infeasible.

The DOSBox-X normal-core/386 synthetic oracle passes **60
cases** for this body in the shared cluster run, comparing all 14 CPU
words, the 128-byte frame window, the seed dword, the class byte and the 1,257-byte grid
with no mask. The COM keeps the original unit 0x9E span at `CS:0DE3`,
copies the original SYSTEM generator and FillChar bytes to the absolute
paragraph `0AF8` at start-up, holds the whole original unit-0x33 code
image at one base so every `PUSH CS; CALL` displacement stays valid, and
relocates the eighteen far-call segment words to one stub paragraph by
the loader's arithmetic; the harness checks that its code, fixture table
and absolute SYSTEM copy stay clear of every DS global the bodies touch.
Maskable interrupts are disabled from frame priming through capture. The
result SHA-256 is `2bfa49e1693778bdd4253d6862d0b0a5917474fe7dd0de4f00633f7ee9c9d8a6` with COM segment `0814`;
another DOS allocation changes the hash while retaining parity. Original
bytes, generated assembly/COM, results and logs stay under ignored
`resources/`; public fixtures are synthetic.

```sh
cmake -S . -B resources/build-grid-driver -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-grid-driver --target poolrad_overlay_u33_grid_diagonal_0bde_tests \
  poolrad_overlay_u33_grid_driver_11b4_tests
ctest --test-dir resources/build-grid-driver \
  -R 'poolrad_overlay_u33_grid_(diagonal_0bde|driver_11b4)_tests' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u33_grid_driver_11b4_audit.py --unicorn --native \
  --output resources/u33-11b4-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x33` `+0BDE..+0D12` with `+0B97` folded, `+08A4` composed at two sites and entry 8 at four |
| `source_candidates` | `src/recovered/overlay_u33_grid_diagonal_0bde.c` |
| `signals` | Exact hash/CFG, one aligned caller inside the game-evidenced driver, the documented grid globals, whole-RAM/CPU differential with full instruction coverage and every roll-reachable branch outcome, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Every conditional outcome reached; `+0B97` folded pending its merged model; loaded-state stubs only; natural saved-game reach unmeasured |

No classifier or count ledger is changed. Identity and strict credit
remain the PM's review decision.

## Pinned AF after SHR AL,1: model fixed (2026-10-23, census #1464 family 2)

The model cleared AF after `SHR AL,1` at `+0CA8` (the arithmetic-flag mask cleared it and only CF/PF/ZF/OF were set). Pinned
DOSBox-X sets AF after a non-zero shift (`get_AF` = `count & 1Fh`, `flags.cpp:172-177`; stored `763-771`). Fix: the model sets AF.

- **Did the evidence depend on AF?** No. The credit rests on `u33_grid_driver_11b4_audit.py`, which composes this body under Unicorn
  (AF=0 after SHR). It now uses the shared declared split (`pinned_shift_flags`): the `+0BDE` part passes **122 cases** with the split
  used at **0** stops, because the SHR's AF never reaches a compared stop.
- **Caller-observable?** No. On the `JAE` path, `INC BYTE [BP-4]` at `+0CB9` overwrites AF; on the fall-through path, the call to
  `+0B97` overwrites it with `ADD DI,AX` at `+0BAD`. Both were walked over the original bytes with pinned semantics.
- **Evidence for the verdict (par-e decides):** the evidence did not depend on AF and the difference is not observable, so we expect
  the credit to stand.

