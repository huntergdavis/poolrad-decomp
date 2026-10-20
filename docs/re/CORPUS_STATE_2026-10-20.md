# Corpus state at main 3dbaf626 (2026-10-20), for a cold start

One page. Every figure below was read on 2026-10-20 from the named file on main
`3dbaf626` (after #1230), or is cited to the document that measured it. Where a
number is a floor, it is called a floor. Re-derive the readiness figures first if
the strict set has grown (`IDENTITY_CLEARED_PRICING.md` says the same).

## 1. Credited

- **251 strict full-C game routines** (225 overlay, 26 resident) and **297 verified
  identities** (270 overlay, 27 resident), from `function_accounting_checkpoint.json`
  after #1227; 251 data rows in `strict_full_c_addresses.csv`. Both whole-game
  denominators are unknown. The 815-body figure is a ceiling on game-tier functions,
  not a denominator (task t6).
- Credited in the U38/U3B0/U25 work this lane touched: `OVL_0038_3134`, `OVL_0038_101D`,
  `OVL_0038_2E08`, `OVL_03B0_11F3`, `OVL_0025_1CC3`, and the installer `OVL_0038_31A2`
  (present in both CSVs). Read status from the CSVs, never from prose.

## 2. Evidence at the bar, **not credited** (author evidence only)

| Row | PR | Cases / coverage | Status |
| --- | --- | --- | --- |
| `OVL_03B0_07A4` | #1202 | 105; caller 115/115, 8/8 | merged, evidence-only |
| `OVL_03B0_0A08` | #1204 | 202; 107/107, 2/2 | merged, evidence-only |
| `OVL_03B0_0C33` | #1209 | 118; 49/49, 6/6 | merged, evidence-only |
| `OVL_03B0_06FB` | #1210 | 63; 13/13 | merged, evidence-only |
| `OVL_03B0_01DA` | #1216 | 95; 171/171 feasible starts, 15/15 outcomes; extent repaired 483 to 419 B | merged, evidence-only |
| `OVL_0038_12B2` | #1224 | 65,814 cases (65,536 exhaustive data-byte pairs; 278 edge/sampled); 48-case native | merged, evidence-only; credit acceptance is the reviewer's |
| `RES_0464_248E` | par-b branch | 41 cases; all figures reproduced | not gated, branch cut from old main |
| `OVL_03B0_1140` | n/a | identity decision (b): its list head dies in stack cells | caveat: I read 0D9A's stores in bytes, did not run it |

None of these is in either credit CSV and none has a manifest declaration; whether to
credit is the reviewer's and VP's decision. Precision limits that must travel with
them: the "1 MiB" compare is 983,040 RAM bytes plus a replay of the aperture writes
(U3B0 rows, 248E); native runs compare only the regions named in each doc (12B2:
10,240 B; 248E: 132,096 B) and used the host DOSBox-X 2026.01.02, not the pinned
`5a9c7c53` build.

## 3. Priced and declined (floors, readiness axis)

VP working ceiling is 132 B per credited row (i19611). Floors are one hop unless noted.

| Row | Floor | Why it stays declined |
| --- | ---: | --- |
| `RES_0464_0F08` | >= 3,176 B, unbounded | indirect callee: one hop is not a bound (stop rule); 0D9A alone is 784 B |
| `OVL_006F_0482` | >= 2,934 B | needs 01C8, 1E5C, 094A, 095D; chain PARKED, do not start 1E5C or 0111 |
| `OVL_006F_1262` | >= 1,876 B (>= 4,312 B with 0482's callees) | needs 0482 and 0EC4 |
| `OVL_0077_299C` | >= 1,011 B (>= 1,795 B with 0D9A) | needs 008A, 02E8 |
| `OVL_006F_0010` | >= 627 B + 5 unpriced endpoints | `runtime_unmapped` callees |
| `OVL_0025_0511` | >= 303 B | two hops; cheapest priced chain |
| `OVL_009E_0026` | >= 38 B + dispatch closure | `indirect_unresolved` callee |

Remaining rows of the pricing table (0411, 1684, 01C8, 005E_0123, 22D5) are all above
900 B. The four 08AE-reaching rows (0482, 299C, 0010, 1262) are also gated by the
policy below.

## 4. Policy-closed

- **08AE band**: 53 rows in the 0x38 band, three-hop floor 1,932 B, closed by the
  owner's policy excluding device-facing driver internals (`RES_08AE_IDENTITY_POLICY.md`,
  VP i19467). "Closed" means unavailable as game credit, not impossible to execute.
- **`RES_0464_2A0D`** (BIOS INT 10h subject) stays refused under the two-factor standard.
- **0482 chain**: parked by VP ruling.

## 5. First actions for the next session

1. #1224 (`OVL_0038_12B2`) is merged as author evidence. Read both credit CSVs to see
   whether the reviewer's acceptance has landed; do not take it from prose.
2. Re-run the pricing derivation on current main before pricing anything; the strict set
   grew by 1CC3 since the table was written.
3. Open task: t6 (strict C toward the ceiling). t24 (`RES_00BA_16EC`) is blocked on the
   board: its 1,193 B and 53-row figures were a task title, never measured, so treat
   them as unknown and measure before using them.
4. Known unswept class: the RETF stack-top wrap (96 source files, `REVIEW_LANE_HANDOFF.md`).
   Unicorn cannot expose it; only a DOSBox-X probe can.
5. Rules that bit this lane: spans from GAME.OVR bytes, not queue rows; credit status from
   the CSVs; PR numbers and heads copied from `gh pr list`; every heavy run through
   `poolrad-heavy.sh`; checker registered and gated on current main before handoff.
