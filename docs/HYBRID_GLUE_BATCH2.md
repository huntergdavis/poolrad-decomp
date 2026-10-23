# Hybrid glue, batch 2: the next four credited bodies on the save walk (VP i20871)

Batch 2 of the standing glue rule (VP i20767). It glues the next four credited bodies on the save walk, by arrival
count, that have a raw-CPU C entry:

| Body | Arrivals on the walk |
|---|---|
| `OVL_0025_0136` | 54 |
| `OVL_0025_0000` | 50 |
| `RES_00BA_1037` | 30 |
| `OVL_0025_0E0C` | 19 |

The bar is the one VP i20871 set: shadow first, and numbers are quoted only from a route run that did not diverge. The
route, save and keys are those of [HYBRID_STAGE_B_OBSERVER.md](HYBRID_STAGE_B_OBSERVER.md). The batch-1 bodies are
in [HYBRID_GLUE_BATCH1.md](HYBRID_GLUE_BATCH1.md).

## The numbers

**Shadow (V1), run `hybrid-twenty-v1-walk`, all 20 bodies dispatched: every checked arrival is equal, with 0
differences.**

| Body | Kind | Checked equal |
|---|---|---|
| `OVL_0025_0136` | RAM-only, composes `0E0C` | 54 of 54 |
| `OVL_0025_0000` | RAM-only, SYSTEM CS from the resident load base | 50 of 50 |
| `RES_00BA_1037` | RAM-only, composes SYSTEM Str and StrStore | 30 of 30 |
| `OVL_0025_0E0C` | RAM-only | **no standalone arrival**: all 19 calls come from inside `0136` |

- **The batch-1 bodies** held with 0 differences in the same run.
- **The one TIMEOUT** is again `RES_0899_0059`'s last ReadKey after Up: it waits for a key that never comes, and the
  original was kept.
- **The final frame** has 0 differing pixels against the unsubstituted walk c.

**Route, run `hybrid-twenty-route-walk`: did not diverge.**

- **IDs:** no new ID was reached.
- **Counts:** apart from the glued bodies, every count that moved is either one of the three timing counters, or a
  callee now composed inside our C:
  - the timing counters are `RES_0A40_02FD`, `RPRO_08AE0` and `RES_0709_0C3F`. The first two already differ between
    the unsubstituted walks c and d; 0C3F is 131,788 here, against 131,758 in both;
  - the callees are listed in the table below.
- **Screen:** the final frame (14,4 W 10:51) has 0 differing pixels.
- **Returns:** 492 routed returns, 0 stops.

| Body | Unsubstituted (walk c) | Original executed in this route | How |
|---|---|---|---|
| `OVL_0025_06D3` | 139 | **0** | inside this batch's composed C (which body composes it is not attributed per call) |
| `OVL_0025_0136` | 54 | **0** | C-only, 54 routed returns |
| `OVL_0025_0000` | 50 | **0** | C-only, 50 routed returns |
| `RES_00BA_1037` | 30 | **0** | C-only, 30 routed returns |
| `OVL_0025_0E0C` | 19 | **0** | inside `0136`'s C |
| `OVL_0025_06FB` | 37 | 18 | 19 of its calls are now inside our C |
| `OVL_0025_1153` | 1 | **0** | inside our C |
| `OVL_0025_07BC` (not credited) | 6 | 0 | inside our C |

**Credited bodies' original executions on the save walk: 2,453 unsubstituted, 937 after batch 1, 625 after batch 2.**

- **This batch's drop of 312** is:
  - 134 C-only returns (`0136` 54, `0000` 50, `1037` 30);
  - 178 composed calls (`06D3` 139, `0E0C` 19, `06FB` 19, `1153` 1).
- **The 625 that remain** are:
  - 361 from the five shadow bodies, which run the original by design;
  - 264 from credited bodies not glued yet.

## A correction to my scope survey

The survey sent in i20769 classed `OVL_0025_06D3`, `06FB` and `06AF` as "semantic-only" font drawers. That was wrong.

- **What they are:** U25 loader entries. This batch's raw-CPU C composes them, and the route run measures it: 06D3 139 → 0, 06FB 37 → 18.
- **Why the survey got it wrong:** my resolver took every `pr_*` symbol a body's evidence doc *mentions*, and those docs
  mention the font model.
- **What it means for the survey:** its counts (150 raw, 17 bus, 39 semantic-only, 71 unresolved) are an upper bound on
  semantic-only, not a measurement. Each body is checked against its own header before it is glued.

## What this does not show

- **Composed, not glued:** `OVL_0025_0E0C` has no standalone arrival on this route, so its own entry glue is wired but
  unexercised. Its 19 executions were checked inside `0136`'s C.
- **No per-body mutants for these four.** The comparison code is the same one shown able to fail (see #1383, #1390, and
  05CD in batch 1).
- **One route, one save, one step.** Timing is not covered.

## Build and run

- **Entries:** the batch-1 list, plus `pr_u25_slot_value_0136`, `pr_u25_load_operands_0000`, `pr_u25_load_word_0e0c`
  and `pr_res_00ba_1037_cpu`.
- **Models object:** `tools/hybrid/build_models.py` links 57 sources.
- **Binary:** sha256 prefix `1f6703beb24925dc`.
- **Dispatch list:** add `OVL_0025_0136,OVL_0025_0000,RES_00BA_1037,OVL_0025_0E0C`.
- **SYSTEM CS:** overlay bodies that need SYSTEM's run-time CS take it from the most recent resident arrival. If none
  has happened yet, the C is treated as ARGUMENT and the original runs.
