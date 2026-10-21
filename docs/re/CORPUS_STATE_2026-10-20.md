# Corpus state at main 4983d59c (2026-10-20), for a cold start

One page, written 2026-10-20 and corrected after par-e's verification against products
regenerated at its own pin. **Pin: main `4983d59c`** (after #1237) for counts and credit status; §3 closures were regenerated again at main `fed69789` (after #1240). Counts and credit
status were read from the two credit CSVs and `function_accounting_checkpoint.json` at that
commit. Readiness and closure figures in §3 were regenerated for this version from the
pinned products: the canonical queue (`function_queue_audit.py`, queue sha
`1e5dd48106c8…`, equal to the checkpoint pin) and the strict-candidate-queue export
(447 candidates) fed to `price_row_closures.py`. Where a number is a floor it says so.
Rows credited after an earlier version of this page was written are listed as such.

## 1. Credited

- **252 strict full-C game routines** (226 overlay, 26 resident) and **298 verified
  identities** (271 overlay, 27 resident), from `function_accounting_checkpoint.json`
  (queue originals 898, unknown ownership 511). Both whole-game denominators are unknown.
  The 815-body figure is a ceiling on game-tier functions, not a denominator (task t6).
- In the U38/U3B0/U25 work this lane touched, now credited: `OVL_0038_3134`,
  `OVL_0038_101D`, `OVL_0038_2E08`, `OVL_03B0_11F3`, `OVL_0025_1CC3`, `OVL_0038_12B2`
  (accepted after the first version of this page, #1224 via #1232), and the installer
  `OVL_0038_31A2`. Read status from the CSVs, never from prose.

## 2. Evidence at the bar, **not credited** (author evidence only)

| Row | PR | Cases / coverage | Status |
| --- | --- | --- | --- |
| `OVL_03B0_07A4` | #1202 | 105; caller 115/115, 8/8 | merged, evidence-only |
| `OVL_03B0_0A08` | #1204 | 202; 107/107, 2/2 | merged, evidence-only |
| `OVL_03B0_0C33` | #1209 | 118; 49/49, 6/6 | merged, evidence-only |
| `OVL_03B0_06FB` | #1210 | 63; 13/13 | merged, evidence-only; **has a manifest `full` row** (`partial_callee_models.csv`, added in `dc76e201`) |
| `OVL_03B0_01DA` | #1216 | 95; 171/171 feasible starts, 15/15 outcomes; extent repaired 483 to 419 B | merged, evidence-only |
| `RES_0464_248E` | par-b branch | 41 cases; all figures reproduced | not gated, branch cut from old main |
| `OVL_03B0_1140` | n/a | identity decision (b): its list head dies in stack cells | caveat: I read 0D9A's stores in bytes, did not run it |

None of these is in either credit CSV. `OVL_03B0_06FB` is the one with a manifest
declaration (it declares readiness, not credit); I checked the others against
`partial_callee_models.csv` and found no row. Whether to credit any of them is the
reviewer's and VP's decision.

Precision limits that must travel with them:
- **The five U3B0 checkers** (07A4, 0A08, 0C33, 06FB, 01DA) map the video aperture
  `A0000..AFFFF` to an independent Python EGA and compare the **whole 1,048,576-byte
  backing** with `ram_excluded_ranges=[]`, plus the full EGA device state, OUT events and
  VRAM write operands. They are full-backing plus EGA; the earlier 983,040-byte wording
  applied to the pre-re-audit checkers.
- **`RES_0464_248E`** compares 983,040 RAM bytes plus a replay of the aperture writes, not
  the full backing (I verified this for 248E; par-e reports the same for `RES_00BA_1951`,
  which I did not re-check).
- Native runs compare only the regions each doc names (248E: 132,096 B) and used the host
  DOSBox-X 2026.01.02, not the pinned `5a9c7c53` build.
- `OVL_0038_12B2` (now credited): 65,814 cases (65,536 exhaustive data-byte pairs; 278
  edge/sampled); 48-case native over three regions (10,240 B).

## 3. Priced and declined (floors, readiness axis)

VP working ceiling is 132 B per credited row (i19611). "Floor" columns are the one-hop
figures in `IDENTITY_CLEARED_PRICING.md`; "Closure" is `price_row_closures.py` on the
regenerated pin (all callees transitively, bodies counted once, endpoints left unpriced; since #1240 declared boundaries `RES_0A40_02FD`, `RES_0A40_030F`, `RES_0AF8_16BC` are terminals and their 12 CRT bodies / 1,161 B are no longer priced, which lowered every 08AE-gated row by exactly 1,161 B / 12 bodies). Queue sha `1e5dd48106c8…` at that commit.

| Row | One-hop floor | Closure (B / bodies) | 08AE-gated | Note |
| --- | ---: | ---: | --- | --- |
| `OVL_001C_0F08` | none sourced | 14,528 / 47 (10 uncovered SYSTEM endpoints) | yes (00DC, 017B) | no indirect dispatch in the closure. An earlier figure of ">= 3,176 B" quoted here does not reproduce from any product or document and is withdrawn; the id `RES_0464_0F08` was also wrong |
| `OVL_006F_0482` | >= 2,934 | 7,468 / 24 | yes | chain PARKED by VP ruling; do not start 1E5C or 0111 |
| `OVL_006F_1262` | >= 1,876 (>= 4,312 with 0482's callees) | 8,952 / 27 | yes | needs 0482 and 0EC4 |
| `OVL_0077_299C` | >= 1,011 (>= 1,795 with 0D9A) | 7,610 / 17 | yes | needs 008A, 02E8 |
| `OVL_006F_0010` | >= 627 + 5 unpriced endpoints | 956 / 6 | yes | `runtime_unmapped` callees |
| `OVL_0025_0511` | >= 303 | 8,120 / 28 | yes | cheapest by one-hop floor, **not** buyable by translation alone |
| `OVL_0025_0411` | >= 926 | 3,148 / 17 | yes | |
| `OVL_0025_1684` | >= 1,499 | 5,908 / 14 | yes | |
| `OVL_03CE_01C8` | >= 1,990 | 6,898 / 21 | yes | |
| `OVL_006F_22D5` | >= 2,404 | 8,989 / 21 | yes | |
| `OVL_009E_0026` | >= 38 + dispatch closure | 314 / 1 | no | dispatch through itself (`indirect_unresolved`) |
| `OVL_005E_0123` | >= 2,029 | 2,549 / 3 | no | dispatch via `OVL_009E_0160`; 009E_02FA is on hold (i19596) |

**Pricing conclusion.** Every priced row except `009E_0026` and `005E_0123` has
`08AE:00DC` and `08AE:017B` in its closure, reached through the resident gateways
`RES_0899_0000` and `RES_0899_0059`. That includes `0511`, which the one-hop table ranks
cheapest. Under the 08AE policy (§4) those rows cannot be credited by translation alone;
the two that are 08AE-free are blocked by indirect dispatch and a held callee.

## 4. Policy-closed

- **08AE band** (`RES_08AE_IDENTITY_POLICY.md`, VP i19467): closed by the owner's policy
  excluding device-facing driver internals. The receipt's opening sentence calls it the
  "53-row 0x38 band" with a measured three-hop floor of 1,932 B. Re-measured here on the
  regenerated export: **53 candidate rows list `RES_00BA_16EC` as a callee, across
  units, of which 21 are in unit 0x38** (that unit has 57 candidates); so "53 rows" is
  right as a count of 16EC-gated rows and wrong as a count of the 0x38 band. The 1,932 B
  floor is the receipt's figure and was not re-derived. "Closed" means unavailable as game
  credit, not impossible to execute.
- **`RES_0464_2A0D`** (BIOS INT 10h subject) stays refused under the two-factor standard.
- **0482 chain**: parked by VP ruling.

## 5. First actions for the next session

1. Read both credit CSVs and the checkpoint; do not take credit status from prose, this
   page included.
2. Re-run the pricing derivation (`function_queue_audit.py`, then
   `strict_candidate_queue.py`, then `price_row_closures.py`) on current main before
   pricing anything; the strict set grows with every acceptance.
3. Open task: t6 (strict C toward the ceiling). t24 (`RES_00BA_16EC`) is blocked on the
   board and its task title's 1,193 B / 53-row figures were never measured by the person
   who wrote them; the 53 above is my count of rows listing 16EC, not that figure.
4. Known unswept class: the RETF stack-top wrap (96 source files, `REVIEW_LANE_HANDOFF.md`).
   Unicorn cannot expose it; only a DOSBox-X probe can.
5. Rules that bit this lane: spans from GAME.OVR bytes, not queue rows; credit status from
   the CSVs; PR numbers and heads copied from `gh pr list`; real clock from `date`, not
   from memory; every heavy run through `poolrad-heavy.sh`; checker registered and gated
   on current main before handoff; summaries verified against regenerated products.
