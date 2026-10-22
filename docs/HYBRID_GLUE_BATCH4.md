# Hybrid glue, batch 4: the credited bodies left on the save walk (VP i21174)

Batch 4 of the standing glue rule, held to the same bar as before: shadow-equal on the walk, per-body counts, the pixel
result, and no number quoted from a route run that diverged. The route, save and keys are those of
[HYBRID_STAGE_B_OBSERVER.md](HYBRID_STAGE_B_OBSERVER.md).

**What was left.** After batch 3, the walk reached 25 credited bodies that were not glued:

| Group | Count | Bodies |
|---|---|---|
| Raw-CPU or `pr_bus` C entry: **glued here** | 17 | see the table below |
| Semantic-only: the C takes a u3bd state struct, not a CPU, so an adapter is needed | 5 | `OVL_03BD_047B` (109 arrivals), `03BD_0155`, `03BD_057A`, `03BD_0350`, `0068_068D` |
| No standalone C entry: reached only inside composed C | 3 | `OVL_0025_06D3`, `06FB`, `18EB` (`06D3` already went 139 → 0 that way in batch 2) |

## The numbers

**Shadow (V1), run `hybrid-b4c-v1-walk`, all 41 bodies dispatched: 0 differences.**

The final frame has 0 differing pixels against the unsubstituted walk c.

| Body | Checked arrivals |
|---|---|
| `RES_0802_0716` (composed entry, DOS read/filepos/seek/open/close replay) | 51 equal; 9 stops (named below) with the original kept; 11 interrupted |
| `OVL_0025_06AF` | 18 equal |
| `OVL_006F_1446` | 5 equal, plus 1 mid-body hand-back (C stopped at callee `08D0:0025`, the original finished, equal) |
| `OVL_03CE_014E` | 6 equal |
| `OVL_03B4_00C7`, `00FB`, `012F` | 3, 3 and 3 equal |
| `OVL_001C_00F4` | 2 equal |
| `OVL_0025_01B6`, `OVL_007D_02F2`, `OVL_03B9_048F`, `OVL_001C_18F8` | 1 equal each |
| `RES_0709_0DD3` | 0 checked: its one arrival was interrupted |
| `OVL_009B_0000`, `0255`, `0329`, `OVL_0025_1153` | no standalone arrival: only reached inside other glued C |

The earlier 24 bodies held with 0 differences.

**0716's nine stops, named.**

- **7 service mismatches,** all a single bit: **PF** in the flags handed to `INT 21h` at `0AF8:1529`, BlockRead's read.
  This is a deviation between SYSTEM BlockRead's model and the pinned DOSBox-X; it is described below.
- **2 `CALLEE` stops at seam `08BC`,** 0716's GetMem call (`pr_r0802_0020`).

**Route, run `hybrid-b4d-route-walk`, with `PR_HYBRID_ROUTE_SKIP=RES_0802_0716`: did not diverge.**

- **IDs:** no new ID was reached.
- **Screen:** the final frame has 0 differing pixels.
- **Returns:** 534 routed returns. The one routed stop is `1446`'s callee stop, which was handed back.
- **Uncredited counts that moved:**
  - the timing counters (`RES_0A40_02FD`, `RES_0709_0C3F`, `RPRO_08AE0`);
  - three callees now inside `0DD3`'s C: `RES_0709_00B7` 2 → 0, `RPRO_07090` 2 → 0, `RPRO_07130` 5 → 1.

**Credited bodies' original executions on the save walk.** These numbers are recomputed against today's credited list
(280 rows). Totals from different lists are not comparable, which is why the earlier batch docs give 581 after batch 3
and this one gives 652.

| Run | Original executions |
|---|---|
| Unsubstituted walk c | 2,524 |
| After batch 3 (`hybrid-b3-route-walk`) | 652 |
| After batch 4 (`hybrid-b4d-route-walk`) | **586** |

The drop of 66, body by body:

| Body | Unsubstituted | After batch 3 | After batch 4 |
|---|---|---|---|
| `OVL_0025_06AF` | 26 | 26 | 0 |
| `OVL_009B_0000` | 6 | 6 | 0 |
| `OVL_009B_0255` | 6 | 6 | 0 |
| `OVL_03CE_014E` | 6 | 6 | 0 |
| `OVL_006F_1446` | 6 | 6 | 1 (the hand-back) |
| `OVL_03B4_00C7` | 3 | 3 | 0 |
| `OVL_03B4_00FB` | 3 | 3 | 0 |
| `OVL_03B4_012F` | 3 | 3 | 0 |
| `OVL_001C_00F4` | 2 | 2 | 0 |
| `OVL_009B_0329` | 1 | 1 | 0 |
| `OVL_0025_01B6` | 1 | 1 | 0 |
| `OVL_007D_02F2` | 1 | 1 | 0 |
| `OVL_03B9_048F` | 1 | 1 | 0 |
| `RES_0709_0DD3` | 1 | 1 | 0 |
| `OVL_001C_18F8` | 1 | 1 | 0 |

## Two findings

**1. A shadow parent undoes C-only children, so route mode skips it.**

- **The effect.** A service-bound body always runs the original first (shadow), and every glued body it calls inside
  that window also runs as original bytes.
- **Measured.** With 0716 dispatched in route (`hybrid-b4b-route-walk`), `RES_0802_050F` went from 0 original executions
  back to 71, and the total rose 652 → 657 instead of falling.
- **The fix.** `PR_HYBRID_ROUTE_SKIP` names bodies that are checked in V1 but not dispatched in route mode. With 0716 on
  it, 050F stays C-only.
- **Rule for later batches.** A service-bound body that calls C-only-capable glued bodies is checked in V1 and skipped
  in route.

**2. SYSTEM BlockRead's MUL PF does not match the pinned DOSBox-X.** Sent to par-a as a corpus-claim item.

- **What the model claims.** `src/recovered/system_blockread_14fb_cpu.c` says MUL's undefined flags are "set as DOSBox-X's
  386 core sets them".
- **What DOSBox-X does.** Its `MULW` sets `PF = PARITY16(AX) ^ PARITY16(DX) ^ FLAG_PF`, where `PARITY16(x)` is itself
  `lookup[hi] ^ lookup[lo] ^ FLAG_PF` (`src/cpu/instructions.h`). The result depends on all four bytes and on the
  previous PF.
- **What the model does.** It takes the parity of the low byte of `AX ^ DX`.
- **Why the end-state differentials missed it.** PF reaches only the flags handed to `INT 21h`, and after the call the
  flags come from the service. So no end-state differential can see it, and the credited composers' end states are
  unaffected. `05CD` passed 71/71 in batch 1 because its reads are small.
- **Suspect too:** `system_seek_1563_cpu.c`, which uses the same parity fold. It has not fired on this route.
- **Not fixed here.** Fixing the model is the owning lane's change.

## What this does not show

- **No checked arrival** for `RES_0709_0DD3` (its one arrival was interrupted), or for the four bodies that are reached
  only inside other glued C.
- **0716 is shadow only, and is skipped in route.** Its 51 equal arrivals are the evidence; the game never runs 0716 as C
  alone.
- **Not glued:** the 5 semantic-only bodies and the 3 that have no entry.
- **One route, one save, one step.** Timing is not covered.

## Build

- **Entries:** the batch-3 list, plus `pr_r0802_0716_composed` and the 16 entry symbols named in the table.
- **Models object:** 85 sources.
- **Binary:** sha256 prefix `fad91cc70835575d`.
- **New in the dispatch:**
  - return detection without a per-body RETF size: pop 0 closes the window at the saved return address once SP has
    risen 4 to 68 bytes;
  - SYSTEM-derived resident 0464 segment for `014E` and `048F`;
  - `PR_HYBRID_ROUTE_SKIP`;
  - service mismatches that name every differing CPU word.
