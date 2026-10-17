# Unit 0x33 +1577 placement search

`pr_u33_place_search_1577` translates the complete DOS v1.3 routine
`GAME.OVR` unit `0x33:+1577..+19F4`. It composes its five near calls to
three models on the same RAM and CPU words:

- the merged [`+1403` both-outside predicate](U33_BOTH_OUTSIDE_1403.md), twice;
- the merged [`+01AD` cell pair](U33_CELL_PAIR_01AD.md), twice, which
  reaches `+012B` and the unit 0x3BD lookups;
- the [`+1434` cell claim](U33_CELL_CLAIM_1434.md), once, which reaches
  unit 0x3C6 `+0CB9`.

It searches an 11 by 6 placement grid around an origin for a position
that `+1434` claims for slot q. The recall attempt for this lane found
nothing to reuse.

## Exact original, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x33 | GAME file base `0xC5D1`; `+1577` is an internal prologue, not a public entry |
| Function | GAME file `0xDB48..0xDFC5`; **1,150 bytes, 470 instructions, 35 conditional branches, five near calls** |
| Function SHA-256 | `c94183327c81204272caf4a0ae4ba6b3881ddad9f01ca60775ee5a154580f288` |
| Near calls | `+17C8` and `+18C8` `CALL +1403`; `+1894` and `+1950` `CALL +01AD`; `+19C7` `CALL +1434`; each preceded by `PUSH CS` |
| Exit | `RETF 2` at `+19F2`; exact end `+19F5` exclusive |

Every branch target lies inside the body, and `+19F5` is the next
prologue. Decoding every unit-0x33 routine from its own prologue finds
exactly one aligned `PUSH CS; CALL +1577`: `+1C75` inside `+19F5`. A raw
`E8` byte at `+180E`, inside `+180D SHR AL,1`, is not an instruction start.
The body reads these DS globals:

- the map byte `DS:4658` and the per-map origin, step-limit and row tables
  `DS:4650`, `4652`, `4654` and `4656`;
- the direction tables `DS:02D2`, `02E2`, `02F2`, `02F6` and `02FE`;
- the step vectors `DS:2736/273F`, the mode byte `DS:49FA` and the party
  position `DS:6AAD/6AAE`.

No gameplay meaning is inferred; "placement search" names the
operation.

## Complete ABI and ordered effects

The API enters after the far CALL with `SS:[SP]` return IP, `[SP+2]`
return CS and `[SP+4]` a word whose low byte is the slot q. `RETF 2`
leaves SP six bytes above entry. The routine pushes BP and reserves 18h
bytes of locals:

- result `[BP-1]`, found `[BP-2]`, first ring `[BP-3]` and exhausted `[BP-4]`;
- outside `[BP-5]`, probe miss `[BP-6]`, state `[BP-7]`, direction `[BP-9]`;
- step vector `[BP-0B/0C]`, ring `[BP-0F]`, step length `[BP-10]`, origin `[BP-11/12]`;
- step count `[BP-13]`, pass `[BP-14]`, base `[BP-15/16]` and position `[BP-17/18]`.

On each pass through the loop head `+15AB`, the direction is
`DS:[2E2 + 4·sign(DS:[4656+map]) + pass]` shifted right once. The state
then drives the walk:

- **State 1** places the base at `DS:[2F6/2FE + 4·(pass>0) + direction]`
  plus the ring times the vector of quadrant direction+2, through `DS:02F2`
  and the step tables.
- **States 2 and 3** step one side and then the other (quadrants +1 and +3)
  by a growing length.

A position outside x 0..`0Ah` or y 0..5 sets `[BP-5]`. In states 2 and 3
a ring ends when the position is outside only one axis (`+1403` returns
0), or when the step count reaches `DS:[4654+map]` on the first ring or
exceeds `0Bh` later. On map 0 with an odd row byte, pass 0 and ring 1,
three `+01AD` probes around the party position (mode `DS:49FA` ≤ 1) can
add a ring.

A position outside both axes starts the next pass. Up to three passes
re-seed the origin from the party position, with an `+01AD` probe when
the mode is ≤ 1. With no pass left, `[BP-4]` is set. An in-grid position
is offered to `+1434(q, x, y, origin, pass)`, whose AL becomes `[BP-2]`.
The loop repeats until `[BP-2]` or `[BP-4]` is nonzero. AL is then 1
unless the search was exhausted, and the final flags are `CMP [BP-4],0`'s.

`[BP-2]` is read before any write when the first positions lie outside
the grid. A stale nonzero byte there ends the search with AL = 1 and
nothing claimed, exactly as in the original.

With degenerate direction tables the original never returns. The host
therefore caps entries into the loop head: on the next entry the model
stops before `+15AB` with every earlier effect kept and status `LIMIT`.
Each near call pushes CS and the return offset relative to the entry IP
and enters the callee model at its offset relative to this entry. The
callee's RETF must return to that frame, else `TRANSFER`. A callee's
`CODE`, `CALLEE` and `ARGUMENT` statuses pass through, with `+0CB9`'s own
status in `*callee_status`. Offsets wrap at 16 bits and physical
addresses at 1 MiB, and this body's RETF frame is read physically
contiguous. Every operand is reread from RAM in source order. Flag writes
that a later instruction always overwrites are folded. Null RAM/CPU or
RAM that is not exactly 1 MiB is rejected before mutation.

## Validation and parity review

The focused C11 test (`-Wall -Wextra -Wpedantic -Werror`, also run under
ASan/UBSan) uses synthetic RAM with only the far-call site and stub
bytes. It covers:

- a claim at the first position (AL = 1 with `CMP`'s flags);
- an exhausted search (AL = 0 and nothing claimed);
- the stale `[BP-2]` exit (AL = 1 and nothing claimed);
- the `LIMIT` cap at `+15AB`;
- the host guards.

The original-byte oracle passes **170 returning cases**, each
comparing the entire 1 MiB RAM and all 14 CPU words with no flag mask.
The image holds the original unit 0x33 code, the unit 0x3BD span
`+031F..+0579`, the three unit 0x3C6 spans and the loaded-state stubs.
The case families are:

- random worlds with cardinal or diagonal quadrant tables;
- worlds with arbitrary quadrant tables and map bytes;
- occupied and claimable neighbourhoods;
- a world whose three `+01AD` probes all return 1;
- a world whose zero perpendicular vectors push a later ring past `0Bh`
  steps;
- two low and three high stack offsets, SS, DS and the grid across A20,
  the grid at offset wrap, a random background and preserved DF.

Three generated worlds whose original never returns stop at the model's `LIMIT` cap and are not compared. Coverage is counted only from compared, returning runs.
It reaches 469 of 470 instruction starts and 68 of 70 conditional
outcomes. The missing start `+17B6` and the outcomes `+1712→+178E` and
`+17B4→+17B6` need a state byte other than 1, 2 or 3 at the dispatch, or
1 at `+17B0`. The state starts at 1, the three states write 2 or 3,
`+18AE` writes 1, and `+18D6`'s 0 either becomes 1 at `+1997` or ends the
search through `[BP-4]`. Only a DS/SS-aliased table write from a callee
could change it, so the audit declares these rather than reaching them.

Lower entry SPs are left out. There the about 70h bytes of nested frames
wrap a callee RETF across `SS:0000`, where the contiguous frame reading
sends the original out of the code and the model reports `TRANSFER`.

The DOSBox-X normal-core/386 synthetic oracle passes **48
cases** for this body and 24 for `+1434` in six runs. Each
run's COM bakes one generated world at its real DS offsets and holds the
original unit 0x33 code, so every `PUSH CS; CALL` displacement is
original. It also holds the unit 0x3BD span and the unit 0x3C6 spans,
placed so that `+0CB9` sits at offset `0CB9` of its own paragraph.

At start-up the COM relocates the three far-call segment words to one
stub paragraph by the loader's arithmetic. For each case it refills the
frame window, restores the availability and slot ranges from pristine
copies, sets the map, mode and arguments, and enters with interrupts
disabled. It then captures all 14 CPU words, the frame window and both
DS ranges with no mask. Only cases whose search ends under the C model
run natively. The combined result SHA-256 is `efaa79df3cb2a675ed6ed90bd20a0459ea93bcaa05b8710a10a18cc5a85e44a9` with COM
segment `0814`.

Original bytes, generated assembly/COM, results and logs stay under
ignored `resources/`; public fixtures are synthetic.

```sh
cmake -S . -B resources/build-place-search -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-place-search --target poolrad_overlay_u33_cell_claim_1434_tests \
  poolrad_overlay_u33_place_search_1577_tests
ctest --test-dir resources/build-place-search \
  -R 'poolrad_overlay_u33_(cell_claim_1434|place_search_1577)_tests' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 5400s \
  python3 tools/overlay/u33_place_search_1577_audit.py --unicorn --native \
  --output resources/u33-1577-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x33` `+1577..+19F4` with `+1403`, `+01AD` and `+1434` composed |
| `source_candidates` | `src/recovered/overlay_u33_place_search_1577.c` |
| `signals` | Exact hash/CFG, one aligned caller, the documented globals, whole-RAM/CPU differential with full coverage of every non-aliased start and outcome, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Two outcomes and one start are reachable only through DS/SS aliasing; degenerate tables never return (host LIMIT cap); `+1434` composes the uncredited `+0CB9` model; natural saved-game reach unmeasured |

No classifier or count ledger is changed. Identity and strict credit
remain the verifying lane's decision.
