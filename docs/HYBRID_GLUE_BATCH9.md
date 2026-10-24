# Hybrid glue, batch 9: the U25 leaves 06D3, 06FB and 18EB (PM i22530)

Three new dispatch rows, all raw:

| Row | C entry | Return | Source |
| --- | --- | --- | --- |
| `OVL_0025_06D3` | `pr_u25_char_remap_06d3_cpu` | RETF 2 (pop 6) | #1516 carve-out |
| `OVL_0025_06FB` | `pr_u25_address_class_06fb_cpu` | RETF 2 (pop 6) | #1516 carve-out |
| `OVL_0025_18EB` | `pr_u25_compare_words_18eb_cpu` | RETF 4 (pop 8); composes SYSTEM FillChar | #1514 new raw model |

**Build:** `a1f6a684`, on main `be4827a3`, with 119 model entries and 130 dispatch rows.

**Every run is pinned** (`PR_HYBRID_SEED_PIN=12345678`, [re/RANDOM_GENERATOR.md](re/RANDOM_GENERATOR.md)) and compared
with the pinned all-original runs of the same seed. **These are test-harness runs.**

## V1 (shadow)

| Run | driver_rc | Non-equal | `06FB` | `18EB` | RandSeed seq / final pixels |
| --- | --- | --- | --- | --- | --- |
| `hybrid-b9-v1-pin-r3` | 0 | 0 | 845 / 845 equal (+2 INTERRUPTED) | 714 / 714 equal (+6 INTERRUPTED) | `ad42d362` / `1a371fdb` = `hybrid-pin2-r3-a` |
| `hybrid-b9-v1-pin-walk` | 0 | 0 | 18 / 18 | 15 / 15 | `ef797c81` / `955fa8ff` = `hybrid-pin2-walk-a` |

**INTERRUPTED** means a hardware interrupt fell inside the original's window. The original result was kept and nothing
was compared. Stub declines: 0.

## Route mode (REPLACED)

| Run | driver_rc | Routed returns | `06FB` | `18EB` | Final frame |
| --- | --- | --- | --- | --- | --- |
| `hybrid-b9-route-pin-walk` | 0 | 613 | 18 | 15 | = `hybrid-pin2-walk-a`, with the RandSeed seq equal too |
| `hybrid-b9-route-pin-r3-b` | 0 | 27,435 (1 handed back: the `OVL_006F_1446` cold stub) | 847 | 720 | 0 of 786,432 px differ from `hybrid-pin2-r3-a` |

Route 3 in route mode shows 60 post-pin draws, equal to the reference's first 60. The other draws are made inside
routed C, so they are not observed. `06FB` and `18EB` are added to
[re/replaced_bodies.json](re/replaced_bodies.json).

## `OVL_0025_06D3`: no own arrivals

`06D3` had **0 own arrivals in all four runs.** On these routes its only callers are the glued `+1153`/`+12FB` string
cores, which run it composed in C. It is therefore glued, but not verified by its own arrivals, and it is not listed as
replaced.

## The first route-3 route-mode run was a driver mislabel (exit 9 -> exit 11)

`hybrid-b9-route-pin-r3` stopped with driver_rc 9, "no encounter within the seek budget", after only 72 of 150 seek
steps. The host load was about 30, so OCR polls ran about 26 s apart. A step's prompt never read the same twice within
`settle`'s 60 s. The driver then took the empty text as an encounter (`not readable`), and the falsy result ended in
exit 9.

**No divergence:** its 37 visible draws are an exact in-order prefix of the reference.

**The fix** is in `tools/hybrid/walk_driver.py`. A seek step whose prompt does not settle is settled again for 180 s,
and if it still does not settle the driver stops with **exit 11** ("did not settle"), not exit 9. The rerun,
`hybrid-b9-route-pin-r3-b`, completed.
