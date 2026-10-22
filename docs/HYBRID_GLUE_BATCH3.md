# Hybrid glue, batch 3: four more credited bodies on the save walk

Batch 3 of the standing glue rule (VP i20767), held to the same bar as before (VP i20871): shadow first, and numbers are
quoted only from a route run that did not diverge. The route, save and keys are those of
[HYBRID_STAGE_B_OBSERVER.md](HYBRID_STAGE_B_OBSERVER.md). The earlier batches are
[HYBRID_GLUE_BATCH1.md](HYBRID_GLUE_BATCH1.md) and [HYBRID_GLUE_BATCH2.md](HYBRID_GLUE_BATCH2.md).

| Body | Walk arrivals | Kind | Return |
|---|---|---|---|
| `OVL_001C_0B52` | 13 | RAM-only; needs unit 0x25's and SYSTEM's run-time CS | RETF |
| `OVL_006F_032F` | 12 | RAM-only | RETF 8 |
| `RES_00BA_1816` | 10 | `pr_bus`; composes the printer `0626` | RETF 0Ah |
| `OVL_0025_1EA6` | 9 | RAM-only; needs SYSTEM's CS | RETF |

## The numbers

**Shadow (V1), run `hybrid-b3-v1-walk`, all 24 bodies dispatched: every checked arrival is equal, with 0 differences.**

| Body | Result |
|---|---|
| `0B52` | 13 of 13 equal |
| `032F` | 12 of 12 equal |
| `1816` | 10 of 10 equal: the device stream (VRAM reads with values, writes, OUTs) and the full state |
| `1EA6` | no standalone arrival: all 9 of its calls come from inside another glued body's window |

- **The earlier 20 bodies** held with 0 differences.
- **The one TIMEOUT** is again `RES_0899_0059`'s last ReadKey after Up.
- **The final frame** has 0 differing pixels against the unsubstituted walk c.

**Route, run `hybrid-b3-route-walk`: did not diverge.**

- **IDs:** no new ID was reached.
- **Returns:** 511 routed returns, 0 stops.
- **Screen:** the final frame has 0 differing pixels.
- **Counts that moved since batch 2:**
  - the three timing counters (`RES_0A40_02FD`, `RES_0709_0C3F`, `RPRO_08AE0`);
  - the new bodies;
  - calls that are now made from inside the new C.

| Body | Unsubstituted (walk c) | Original executed in this route | How |
|---|---|---|---|
| `OVL_001C_0B52` | 13 | **0** | C-only, 13 routed returns |
| `OVL_006F_032F` | 12 | **0** | C-only, 12 routed returns |
| `RES_00BA_1816` | 10 | **0** | C-only on the real VGA, 10 routed returns |
| `OVL_0025_1EA6` | 9 | **0** | inside our C |
| `RES_0709_0626` | 96 | 0 (unchanged since batch 1) | 10 of its arrivals are now inside `1816`'s C, so it has 86 standalone arrivals |
| `OVL_0025_0000` | 50 | 0 (unchanged since batch 2) | 6 of its arrivals are now inside the new C, so it has 44 standalone arrivals |

**Credited bodies' original executions on the save walk:**

| Stage | Executions |
|---|---|
| Unsubstituted | 2,453 |
| After batch 1 | 937 |
| After batch 2 | 625 |
| After batch 3 | **581** |

- **This batch's drop of 44:** 13 + 12 + 10 C-only returns, plus 9 composed `1EA6` calls.
- **The 581 that remain:**
  - 361 from the five shadow bodies, which run the original by design;
  - 220 from credited bodies not glued yet.

## Notes

**Overlay code segments come from live arrivals.**

- The dispatch records each overlay unit's run-time CS from the most recent observer arrival of any of that unit's
  registry IDs.
- SYSTEM's CS is taken from the most recent resident arrival.
- A body whose needed segment has not been seen yet runs as ARGUMENT, and the original runs instead. An overlay that is
  later reloaded at another segment could leave a stale value. The model would then stop, and the comparison would
  report it. That did not happen on this route.

## What this does not show

- **`1EA6` is composed, not exercised standalone.** Its entry glue is wired but unexercised on this route.
- **No per-body mutants for these four.**
- **One route, one save, one step.** Timing is not covered.

## Build

- **Entries:** the batch-2 list, plus `pr_u1c_if_0b52`, `pr_u6f_vector_merge_032f`, `pr_r00ba_display_1816_cpu` and
  `pr_u25_skip_statement_1ea6`.
- **Models object:** 61 sources.
- **Binary:** sha256 prefix `fb0028753fa2c193`.
- **Dispatch list:** add `OVL_001C_0B52,OVL_006F_032F,RES_00BA_1816,OVL_0025_1EA6`.
