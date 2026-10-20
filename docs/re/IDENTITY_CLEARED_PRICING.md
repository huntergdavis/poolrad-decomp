# Identity-cleared rows: measured readiness prices (2026-10-20)

The class-(a) identity sweep ([IDENTITY_SWEEP_CLASS_A.md](IDENTITY_SWEEP_CLASS_A.md))
plus the VP's edge rulings (i19596) left a work queue of thirteen rows. This
record prices them by **readiness**, which is a separate axis from identity.
A row can be identity-cleared and still not buyable, because its callees are
not ready. `OVL_009E_0026` is the cleanest example: its identity is
established, and its readiness is blocked by an indirect dispatch.

**Decision recorded (VP i19611): decline these chains on cost.** Only
`OVL_03B0_11F3` is actionable (0 unready). Do not start `RES_0464_0111`,
`RES_0464_1E5C` or the U3B9 leaves on the strength of this table. Re-derive
the table first if the strict set has grown.

## How the figures were measured

- Queue: `strict_candidate_queue.py` export regenerated on main `8e82b54a`
  (2026-10-20 12:19–12:22 PDT). The export file (ignored) has SHA-256
  `a89e59e1be77033ff92ea91df99952a8d3aa57d49cbf019e3556fc9e67d65da2`.
- **Span**: decoded from GAME.OVR to the first return that no earlier direct
  branch jumps past. A queue extent larger than this span is flagged.
- **Unready**: every callee whose readiness is not
  `strict`/`modeled`/`resident_covered`/`runtime_covered`, using the code's
  own readiness sets, never a hand list.
- **Floors**: an overlay callee is priced at its queue row bytes. A resident
  callee is priced at its first-return span in the loader-expanded START.EXE.
  `runtime_unmapped` endpoints are left **unpriced**.
- **Direct price floor**: the sum over a row's unready callees, one hop only.
  Every unready callee listed here has unready callees of its own (the
  "own" column), so **all prices are floors and errors run upward**, as
  [REVIEW_LANE_HANDOFF.md](REVIEW_LANE_HANDOFF.md) warns.
- Every row already has three or four implementation files on main (the
  queue's `prior_translation.main_files`), so any purchase is a full-bar
  upgrade, not a new body.

## Per-row prices

| Row | Span (B / insns) | Class | Unready | Unready callees (floor B; own unready) | Direct floor |
| --- | ---: | --- | ---: | --- | ---: |
| OVL_03B0_11F3 | 82 / 29 | helper_candidate | 0/1 | — | **0 (actionable)** |
| OVL_0025_1CC3 | 301 / 81 | helper_candidate | 3/3 | three `runtime_unmapped` endpoints (unpriced) | ≥0 + 3 unpriced |
| OVL_006F_0010 | 391 / 147 (queue 424) | game_evidenced | 7/14 | 006F_02AE 129 (2/7); RES_0802_02EB ≥107; five `runtime_unmapped` (unpriced) | ≥236 + 5 unpriced |
| OVL_0025_0511 | 51 / 21 (queue 61) | unresolved | 2/2 | 03B9_05D9 190 (1/5); 03B9_0697 113 (3/3) | ≥303 |
| OVL_009E_0026 | 282 / 94 (queue 314) | unresolved | 1/2 | 009E_0000 38, `indirect_unresolved` (1/1) | ≥38 + dispatch closure |
| OVL_0077_299C | 369 / 164 | game_evidenced | 2/8 | 0086_008A 515 (2/6); RES_03F1_02E8 ≥127 | ≥642 |
| OVL_0025_0411 | 113 / 36 | unresolved | 1/2 | 006F_0EC4 926 (3/11) | ≥926 |
| OVL_006F_1262 | 452 / 174 (queue 484) | game_evidenced | 2/6 | 006F_0482 540 (4/10); 006F_0EC4 926 (3/11) | ≥1,466 |
| OVL_0025_1684 | 260 / 100 | game_evidenced | 3/6 | 0025_156D 279 (1/7); 03B0_03BD 830 (3/11); RES_00BA_0767 ≥390 | ≥1,499 |
| OVL_03CE_01C8 | 820 / 353 | game_evidenced | 2/10 | RES_0464_0111 ≥404; RES_0464_1E5C ≥1,586 | ≥1,990 |
| OVL_005E_0123 | 110 / 36 | game_evidenced | 1/1 | 009E_02FA 2,029, `partial` (1/1; HOLD per i19596) | ≥2,029 |
| OVL_006F_22D5 | 267 / 107 | game_evidenced | 5/9 | 006F_1D58 1,405 (11/21); 03B0_03BD 830 (3/11); RES_0802_02EB ≥107; RES_0802_095D ≥11; RES_0899_0000 ≥51 | ≥2,404 |
| OVL_006F_0482 | 498 / 183 (queue 540) | game_evidenced | 4/10 | 03CE_01C8 820 (2/10); RES_0464_1E5C ≥1,586; RES_0802_094A ≥19; RES_0802_095D ≥11 | ≥2,436 |

`unresolved` abbreviates `game_or_other_library_unresolved`. For comparison,
the VP's working ceiling is 132 B per credited row (i19611). The cheapest
priced chain above (0511) is ≥303 B for one row and needs two hops.

## Shared blockers

Buying one of these advances more than one row. It still does not complete
any row alone, because every row listed has other unready callees.

| Blocker | Floor | Rows it gates |
| --- | ---: | --- |
| RES_0464_1E5C | ≥1,586 B (604 insns, 17 calls) | OVL_006F_0482, OVL_03CE_01C8 |
| OVL_006F_0EC4 | 926 B (3/11 own unready) | OVL_006F_1262, OVL_0025_0411 |
| OVL_03B0_03BD | 830 B (3/11 own unready) | OVL_0025_1684, OVL_006F_22D5 |
| RES_0802_02EB | ≥107 B (5 SYSTEM calls) | OVL_006F_0010, OVL_006F_22D5 |
| RES_0802_095D | ≥11 B (call-free) | OVL_006F_0482, OVL_006F_22D5 |
| RES_0464_0111 | ≥404 B (154 insns, 15 calls) | OVL_03CE_01C8 directly; OVL_0025_0511 via 03B9_05D9 |

`OVL_03CE_01C8` and `OVL_006F_0482` are also linked: 0482 calls 01C8. Pricing
0482 therefore includes all of 01C8's price.

## Extent findings made while pricing

Four queue extents exceed the first-return span:

- **Trailing data, confirmed:** `OVL_0025_0511` (10 B), `OVL_006F_0482`
  (42 B) and `OVL_006F_0010` (33 B). Each tail is wholly length-prefixed
  printable strings, and none contains an indirect transfer. They are recorded
  with hashes in
  [QUEUE_METADATA_REPAIR.md](QUEUE_METADATA_REPAIR.md#identity-cleared-rows-trailing-data-2026-10-20).
- **Unverified, not proposed:** `OVL_009E_0026` and `OVL_006F_1262` each
  carry a 32-byte tail that is not text (`00 00 20 00 00 60 02 …` and
  `FF 00 …`). These look like Pascal 256-member set constants, but no
  immediate reference to either tail occurs in its body, and the data use
  was not traced. They stay as the queue has them until someone finds the
  referencing instruction.

The static pricing script is not checked in; its output is reproducible from
the queue export and media using the method above.
