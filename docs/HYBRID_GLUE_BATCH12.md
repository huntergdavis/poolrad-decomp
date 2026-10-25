# Hybrid glue, batches 12 and 13: R4's bodies and codex's expanded-observer bodies (PM i22947, i22962, i22989)

Nine bodies with raw or bus entries are glued, with `pop 0`:

| Batch | Route | Bodies |
| --- | --- | --- |
| 12 | R4 (#1535: shop Buy, Sune temple) | `OVL_0077_275B`, `OVL_0077_28BE`, `OVL_007F_00A8`, `OVL_007F_0A88` (bus), `OVL_007F_0B93` |
| 13 | R4 | `OVL_0023_0239` (bus; raw model `overlay_u23_append_0239_cpu`) |
| 13 | R2 | `OVL_006A_0000`, `OVL_006A_025C`, `OVL_006A_0E37` |

**Not glued here:** the four semantic-only bodies the two batches also named, `OVL_007F_0130`, `OVL_007F_0185`,
`OVL_0025_19DB` and `OVL_0025_1815`. par-d2 is writing raw models for them (#1541, #1542, #1543), and they are glued as
those merge.

**Build:** `3569f756`, with 156 model entries and 167 dispatch rows. **Runs:** `run_r1.sh` dispatch mode, pinned at
`12345678`. Each route is compared with codex's all-original pinned pair for that route:

| Route | Final RGBA | Raw-RGB |
| --- | --- | --- |
| R4 | `008650a6…` | `1b7c8943…` |
| R2 | `4474ef30…` | `d612a49e…` |

**These are test-harness runs.**

| Run | Result |
| --- | --- |
| `hybrid-b12-r4-v1` | non-equal 0; final frame and all 33 draws equal. 28BE 3/3, 00A8 2/2, 0A88 1/1, 0B93 1/1, 0239 2/2 |
| `hybrid-b12-r4-route` | 11,019 routed returns; **0 px**; all 33 draws equal. Routed: 28BE 3, 00A8 2, 0A88 1, 0B93 1, 0239 2, and `03B0_0000` 4 (batch 10) |
| `hybrid-b12-r2-v1` | non-equal 0; final frame equal. 6A_0000 6/6, 025C 6/6, 0E37 17/17 (+1 INTERRUPTED) |
| `hybrid-b12-r2-route` | 1,137 routed returns; **0 px**. Routed: 6A_0000 6, 025C 6, 0E37 18 |

- **0 own arrivals on both routes:** `OVL_0077_275B`. It runs composed inside its glued caller, `23_0239`, and is not
  claimed.
- **Hand-backs:** the only hand-back in all four runs is `OVL_006F_1446`, at the cold INT 3Fh stub (`code=CD 3F`).
- **REPLACED:** #1545 adds the 9 routed bodies (112 → 121).

**TIMEOUT lines, all in bodies outside this batch.** A V1 window allows 200,000 instructions. When the original runs
longer than that, the dispatch abandons the window, the original result stands, and nothing is compared. The rows below
are TIMEOUT counts.

| Run | `RES_0899_0059` (ReadKey) | `RES_00BA_1693` (display) |
| --- | --- | --- |
| `hybrid-b12-r4-v1` | 2 (arrivals 1 and 63) | 2 (arrivals 1 and 2) |
| `hybrid-b12-r2-v1` | 1 | 0 |
| `hybrid-b12-r4-route` | 2 | 0 |
| `hybrid-b12-r2-route` | 1 | 0 |

- **`0059`** waits for a key, and with DOS-service replay it stays shadow even in route mode.
- **`1693`** is routed in route mode, so it has no window there.
