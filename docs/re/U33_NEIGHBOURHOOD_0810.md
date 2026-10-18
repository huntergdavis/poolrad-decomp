# Unit 0x33 +0810 neighbourhood sweep

`pr_u33_neighbourhood_0810` translates the complete DOS v1.3 routine
`GAME.OVR` unit `0x33:+0810..+08A3`. It is the mode-1 counterpart of the
[`+11B4` grid driver](U33_GRID_DRIVER_11B4.md); the dispatcher `+1244` calls
it when `DS:49FA` is 1. It composes, on the same RAM and CPU words:

- the merged [`+01AD` cell pair](U33_CELL_PAIR_01AD.md) three times per
  position, through `+012B` and the unit 0x3BD lookups;
- the stamp bodies [`+02BA`](U33_STAMP_BLOCK_02BA.md) and
  [`+0377`](U33_STAMP_SQUARE_0377.md);
- the merged pattern bodies [`+040F`](U33_GRID_PATTERN_040F.md) and
  [`+0615`](U33_GRID_PATTERN_0615.md), all writing through
  [`+0221`](U33_CLIPPED_WRITER_0221.md);
- the [`+0000` grid roll](U33_GRID_ROLL_0000.md), with unit 0x9E entry 8
  and the SYSTEM generator.

The recall attempt for this lane found nothing to reuse.

## Exact original, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Function | GAME file `0xCDE1..0xCE74`; **148 bytes, 71 instructions, two conditional branches, eight near-call sites** |
| Function SHA-256 | `73d7b42bd2b006650d6856afd73118648ac17bd55a7c7eceafe2bf539e5569ac` |
| Near calls | `+0852`, `+0864` and `+0876` `+01AD`; `+087E` `+02BA`; `+0883` `+0377`; `+0888` `+040F`; `+088D` `+0615`; `+089D` `+0000`; each preceded by `PUSH CS` |
| Exit | `RETF` at `+08A3`; exact end `+08A4` exclusive |

Decoding every unit-0x33 routine from its own prologue finds exactly one
aligned `PUSH CS; CALL +0810`: `+12D4` inside `+1244`. No gameplay meaning
is inferred; "neighbourhood sweep" names the loop.

## Complete ABI and ordered effects

The API enters after the far CALL with `SS:[SP]` return IP and `[SP+2]`
return CS and no arguments; `RETF` leaves SP four bytes above entry. The
routine pushes BP and reserves ten bytes:

- dx `[BP-1]` and dy `[BP-2]`;
- x `[BP-3]` and y `[BP-4]`;
- the three `+01AD` results `[BP-7]`, `[BP-8]` and `[BP-9]`.

For dy = −2..2 and dx = −6..6 it forms x = sign(dx) + sign(`DS:6AAD`) and
y = sign(dy) + sign(`DS:6AAE`). It pushes x, y and facings 6, 0 and 2 as
live AX words to `+01AD`, storing AL in `[BP-8]`, `[BP-7]` and `[BP-9]`. It
then pushes its own BP as the static link and calls `+02BA`, `+0377`,
`+040F` and `+0615`; these read dy and dx at the link's −2 and −1 and the
results at −7, −8 and −9. After dy = 2 and dx = 6 it calls `+0000`, whose
final flags become the routine's, then `MOV SP,BP; POP BP; RETF`.

Each near call pushes CS and the return offset and requires the callee's
RETF to return to the frame:

- `+01AD`, `+02BA`, `+0377` and `+0000` are entered relative to the entry
  IP (`0x0810` in the game).
- The merged `+040F` and `+0615` models are entered at their absolute load
  offsets `040F` and `0615`, which they require.

A callee's `CODE` or `TRANSFER` passes through. Any other stop reports
`CALLEE`; `*callee` names the call site and carries the callee's own status,
or the dice chain's for `+0000`.

Every operand is reread from RAM in source order. Offsets wrap at 16 bits,
physical addresses at 1 MiB, and the RETF frame is read physically
contiguous. The loop `CMP`s, the `INC`s and `SUB SP,0Ah` write only flags
that the next callee overwrites. Null RAM/CPU or RAM that is not exactly
1 MiB is rejected before mutation.

## Validation and parity review

The focused C11 test (`-Wall -Wextra -Wpedantic -Werror`, also ASan/UBSan)
uses synthetic site and stub metadata only. It covers:

- a sweep with no eligible cell: the frame unwinds, the seed is untouched
  and the grid is stamped;
- an all-eligible roll, whose seed must be 1,250..3,750 generator steps
  from the start;
- a dice site that is not the original (`CODE` at `+089D`);
- a `+012B` site that is not the original, as `CODE` at the first `+01AD`
  call or as `CALLEE` from inside `+0615`;
- the host guards.

The original-byte oracle passes **68 returning cases**, each comparing the
entire 1 MiB RAM and all 14 CPU words with no flag mask. The image holds the
original unit 0x33 code, the unit 0x3BD and unit 0x9E spans, the SYSTEM
generator at `0AF8:11C0` and the loaded-state stubs. The cases are:

- 48 random worlds (party position, map, seed, lookup tables, eligibility
  density, grid, buffer, flags, ES and every entry register);
- seven party positions including both signed extremes, map 10 and
  all-eligible worlds;
- four stack offsets near both segment ends;
- SS, DS and the grid across A20, the grid at offset wrap, and the grid in
  the stack segment below the frames;
- a random background and preserved DF.

All 71 instruction starts and all 4 conditional outcomes are reached. A grid
laid over `+0810`'s own frame is not a parity case: the stamps rewrite its
loop counters and the original never returns. Frame aliasing that
terminates is exercised in the `+02BA`/`+0377` and `+0615` audits.

The DOSBox-X normal-core/386 synthetic oracle passes **24 cases**, comparing
all 14 CPU words, a 256-byte frame window, the 1,264-byte grid and the seed
with no mask. The COM bakes the DS tables at their real offsets. The unit
0x33 copy sits in its own paragraph, so `+0810`, `+040F` and `+0615` run at
their original offsets. The unit 0x9E copy puts entry 8 at `0DE3`, and the
SYSTEM generator is copied to the absolute paragraph `0AF8` at start-up.

The five far-call segment words are relocated to one stub paragraph by the
loader's arithmetic. The result SHA-256 is
`30c4e92a87221f2b3d6f40b5494d2edb9d0664b948a5c98b33858de242203636` with COM
segment `0814`. Original bytes and generated images stay under ignored
`resources/`.

```sh
cmake -S . -B resources/build-0810 -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-0810 --target poolrad_overlay_u33_neighbourhood_0810_tests
ctest --test-dir resources/build-0810 -R poolrad_overlay_u33_neighbourhood_0810_tests --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 1800s \
  python3 tools/overlay/u33_neighbourhood_0810_audit.py --unicorn --native --output resources/u33-0810-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x33` `+0810..+08A3` with `+01AD`, `+02BA`, `+0377`, `+040F`, `+0615` and `+0000` composed |
| `source_candidates` | `src/recovered/overlay_u33_neighbourhood_0810.c` |
| `signals` | Exact hash/CFG, one aligned caller, whole-RAM/CPU differential through every callee with full coverage, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Depends on #883's `+02BA`/`+0377` and the pending `+0221` credit; a grid over the routine's own frame makes the original non-returning; natural saved-game reach unmeasured |

No classifier or count ledger is changed. Identity and strict credit remain
the verifying lane's decision.
