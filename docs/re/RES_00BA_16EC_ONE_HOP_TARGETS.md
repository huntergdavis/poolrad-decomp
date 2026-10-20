# `RES_00BA_16EC`: loader-derived one-hop target list

**Provenance.** Every number here is measured from the **loader-expanded**
START.EXE image via `resident_loader_map.unpack`, the same path the credited
resident bodies used — not from a sweep of the overlay file, and not from a
queue row. Regenerate with:

    python3 tools/overlay/resident_target_list.py --resident 00BA:16EC --markdown

**Why this file exists.** The strict candidate queue covers non-strict
**overlays only**, so a resident body has no row and no `callee_readiness.targets`
in that export. The rule "check the queue's target list before pricing" is
therefore unsatisfiable for residents, and any resident cost quoted across lanes
has to come from an artifact the receiving lane can regenerate. This is that
artifact. **Overlay-only rows do not apply here.**

## The list

| callee | kind | extent to first `RETF` | starts | readiness | misses? | unlocks alone |
| --- | --- | ---: | ---: | --- | --- | ---: |
| `0AF8:0B25` | runtime | – | – | runtime_covered | – | 0 |
| `RES_03F1_0137` | resident | 433 | 188 | strict | – | 0 |
| `RES_0709_0768` | resident | 635 | 246 | resident_unmapped | yes | 0 |
| `RES_03F1_0000` | resident | 311 | 145 | resident_unmapped | yes | 0 |
| `RES_0709_0DBE` | resident | 21 | 10 | strict | – | 0 |

`RES_00BA_16EC` own extent **247 B** (116 starts, `RETF 0Ch`).

**One-hop total: 1,193 B** = 247 + 635 (`0768`) + 311 (`03F1_0000`).
**Rows this body alone unlocks: 14.**

## Correction to an earlier cross-lane figure

A **1.8 KB one-hop / 14-row** figure was quoted to the other lane earlier from a
sweep-based expansion. The loader-derived figure is **1,193 B**; the 14-row
unlock stands. The sweep was larger because it walked the overlay file rather
than the expanded image and over-counted extents. Treat 1,193 B as the number,
and as a **floor** in the sense recorded elsewhere: it excludes `0768`'s and
`03F1_0000`'s own callees, so a second hop can only raise it.

## Relationship to the other held hubs

Both unready callees are resident leaves that have never been translated
(`RES_0709_0768`, `RES_03F1_0000`), and neither appears in the manifest. So the
cost of 16EC is two resident translations plus its own 247 B — which is why it
is a chain and not a body, even though its own extent is small.
