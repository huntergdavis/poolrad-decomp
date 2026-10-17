# Unit 0x33 +0000 grid roll

`pr_u33_grid_roll_0000` translates the complete DOS v1.3 routine
`GAME.OVR` unit `0x33:+0000..+012A` and composes its three far calls to
unit `0x9E` public entry 8 (`+0DE3`, the strict
[dice roll](U9E_ENTRY8_ROLL.md)) through the overlay stub `00B0:0048`.
It sweeps a 50 by 25 cell grid and rewrites eligible cells by dice. The
recall attempt, branch search and reused work are recorded in
[U2F_PARTY_PICK_0131.md](U2F_PARTY_PICK_0131.md), whose audit covers all
three callers.

## Exact original, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x9E entries 8 and 9 | `+0DE3..+0E51`, 111 bytes, SHA-256 `9a2681e8b24a9a95838a11f2d7cbddea0c253d29e631db37d25fae017a14693b`; stub paragraph `00B0`, offsets `48` and `4D` |
| SYSTEM generator | START.EXE `0xBE36..0xBEBC` (135 bytes): the `+11C0` wrapper (`e4337c9e…`), the `+120F` seed step (`f5b67b3e…`) and the multiplier word `8405` at `+1245`, as the merged [entry-8 roll evidence](U9E_ENTRY8_ROLL.md) pins them |
| Unit 0x33 | GAME file base `0xC5D1`, stub segment `0x003B`; `+0000` is the first prologue, not a public entry |
| Function | GAME file `0xC5D1..0xC6FB`; **299 bytes, 123 instructions, fourteen conditional branches, three far calls** |
| Function SHA-256 | `6d23833ec3c084d77098814aef631263dd7fca54ff1845b6b48bd819926018ee` |
| Far calls | `+004B`, `+00B7` and `+00C6` `LCALL 00B0:0048` (entry 8), each segment word fixup-backed |
| Exit | `RETF` at `+012A`; exact end `+012B` exclusive |

Every branch target lies inside the 299-byte body. The audit decodes each
unit-0x33 routine from its own prologue and finds exactly two aligned
`PUSH CS; CALL +0000` sites: `+0810` at `+089D` and the game-evidenced
`+11B4` at `+1225`. The body reads the far grid `DS:6716/6718` with the
50-byte row stride and `+7` cell byte that the unit-0x33 pair and grid
writers parked on the Codex branches also use, the map id `DS:84DC`
named in [GAMEPLAY_PATH.md](GAMEPLAY_PATH.md), and a four-byte-stride DS
table at `2747`. No gameplay meaning is inferred for the written values
`1Ah`..`1Dh`; "grid roll" names the operation.

## Complete ABI and ordered effects

The API enters after the far CALL with `SS:[SP]` return IP and `[SP+2]`
return CS and no arguments; `RETF` leaves SP four bytes above entry. The
routine pushes BP and reserves four local bytes: column `[BP-1]`, row
`[BP-2]` and the first roll `[BP-3]`.

For column 0..`31h` and row 0..`18h` it forms CX := sign(column),
AX := sign(row) * `32h` with `MUL`, LES the grid pointer, DI := grid +
AX + CX, reads cell byte `+7`, zero-extends it into DI shifted left twice
and compares DS `2747+DI` with `16h`. When equal it rolls (1, `64h`)
through entry 8 and stores AL: 1..`61h` leaves the cell; `62h` and `63h`
recompute the address and write `1Ah` and `1Bh`; `64h`, unless `DS:84DC`
is `0Ah`, rolls (1, `64h`) again and only on exactly 1 rolls (1, `0Ah`),
writing `1Ch` for 1..6 and `1Dh` for 7..`0Ah`. The row compare with `18h`
and the column compare with `31h` reread their live locals; the final
flags are the column compare's. AX, BX, CX, DX, SI, ES and DI return as
the last cell address, lookup or roll left them. Every input returns: the
loops are bounded by their byte counters.

Each far call is modelled as the loaded-state transfer: the relocated
segment word is read from the call site in RAM, the five-byte far jump at
that paragraph's stub offset must name the original entry offset, CS:IP
move to the jump target, and the merged chain runs on the same RAM and CPU
words: `pr_u9e_entry8_fullstate_compose_0de3` (entry 8, its loop, its
SYSTEM call at `0AF8:11C0`, the seed step at `0AF8:120F` reading and
writing the DS dword `4428`, and its RETF 4) with a host cap of 256 call
slices that a byte count can never reach. A site or stub that is not the
loaded-state original reports `CODE` before any effect; a chain stop
reports `CALLEE` with the chain's own status in `*chain_status`; a frame
that no longer returns reports `TRANSFER`. The merged chain requires entry
8 at offset `0DE3` of its segment and SYSTEM at paragraph `0AF8`, which is
where the game loads them. Offsets wrap at 16 bits; words, LES operands
and return frames are physically contiguous; physical addresses wrap at
1 MiB. Null RAM/CPU or RAM that is not exactly 1 MiB is rejected before
mutation.

## Validation and parity review

The focused C11 test (`-Wall -Wextra -Wpedantic -Werror`) runs the merged
chain on synthetic RAM: an all-eligible grid whose 1,250 cells are
predicted cell by cell from the generator formula in the original's
column-major order (including the two-roll `64h` path), a grid with no
eligible cell and an untouched seed, a stub that is not a far jump
(`CODE`) and the host guards.

The original-byte oracle passes **121 returning cases**, each
comparing the entire 1 MiB RAM and all 14 CPU words with no flag mask:
40 random grids, tables, seeds, maps and flags, 60 all-eligible grids with
random seeds and maps (up to 1,250 rolls and their second and third rolls
each), no eligible cell, map `0Ah`, the grid across A20, grid offset wrap,
the grid under the stack, the grid over the seed, eleven stack offsets at
both segment ends, A20 wrap of the stack and DS, a random background and
preserved DF. All 123 instruction starts and 23 of 28 conditional
outcomes are reached; the five untaken outcomes are the ones a 1..range
roll cannot produce (a (1, `64h`) result of 0 or, after 1..`61h`, `62h`
and `63h` are excluded, anything but `64h`; a (1, `0Ah`) result of 0 or
above `0Ah`), and the audit names them explicitly.

The DOSBox-X normal-core/386 synthetic oracle passes **60 cases** for this
body (80 for `+0131`, 80 for `+2D38` and 60 for `+0000` in one run),
comparing all 14 CPU words, 128 frame bytes, the seed dword, the three
latch bytes `6818`, `6819` and `681C` and the 1,257-byte grid with no
mask. The COM keeps the original unit 0x9E span at `CS:0DE3`, copies the
original SYSTEM bytes to the absolute paragraph `0AF8` at start-up (COM
offset `4000` with the observed segment `0814`), holds the three original
caller spans elsewhere and relocates every far call's segment word to one
stub paragraph by the loader's arithmetic. The harness checks that its
code, fixture table and absolute SYSTEM copy stay clear of every DS global
the bodies touch (an earlier layout let the seed dword overwrite one
fixture row). Maskable interrupts are disabled from frame priming through
capture. The result SHA-256 is `2713b2d07c7a47be2771afa2e8d437b1e7a787d4991f7d3bd4148a49826151ba` with COM segment `0814`; another DOS
allocation changes the hash while retaining parity. Original bytes,
generated assembly/COM, results and logs stay under ignored `resources/`;
public fixtures are synthetic, and the public C tests run the merged chain
without any original code bytes.

```sh
cmake -S . -B resources/build-dice-callers -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-dice-callers --target poolrad_overlay_u2f_party_pick_0131_tests \
  poolrad_overlay_u38_record_roll_2d38_tests poolrad_overlay_u33_grid_roll_0000_tests
ctest --test-dir resources/build-dice-callers \
  -R 'poolrad_overlay_(u2f_party_pick_0131|u38_record_roll_2d38|u33_grid_roll_0000)_tests' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 2400s \
  python3 tools/overlay/u33_grid_roll_0000_audit.py --unicorn --native \
  --output resources/u33-0000-dice-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x33` `+0000..+012A` with entry 8 and SYSTEM composed at three sites |
| `source_candidates` | `src/recovered/overlay_u33_grid_roll_0000.c` |
| `signals` | Exact hash/CFG, two aligned callers including a game-evidenced one, the documented grid, map and table globals, whole-RAM/CPU differential with full instruction coverage and every roll-reachable branch outcome, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Five branch outcomes unreachable with 1..range rolls; loaded-state stubs only; natural saved-game reach unmeasured; `+0810` remains an unresolved identity |

No classifier or count ledger is changed. Identity and strict credit
remain the PM's review decision.
