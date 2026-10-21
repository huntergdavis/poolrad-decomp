# Corpus state at main 83c247a4 (2026-10-20, evening), for a cold start

One page, written 2026-10-20 and corrected after par-e's verification against products
regenerated at its own pin. **Counts and credit status: main `83c247a4`** (after #1273; checkpoint queue sha `b4fd0885dce4…`). The readiness figures in §1a and §3 come from a strict-candidate export regenerated on main `3a2b1bd0`, which `load_fresh_export` confirms is current at `83c247a4`. The §3 closure table was regenerated at main `fed69789` (after #1240), and the 0899 reach at `b3fd5718`. The first version of this page was pinned at `4983d59c`. Counts and credit
status were read from the two credit CSVs and `function_accounting_checkpoint.json` at that
commit. Readiness and closure figures in §3 were regenerated for this version from the
pinned products: the canonical queue (`function_queue_audit.py`, queue sha
`1e5dd48106c8…`, equal to the checkpoint pin) and the strict-candidate-queue export
(447 candidates) fed to `price_row_closures.py`. Where a number is a floor it says so.
Rows credited after an earlier version of this page was written are listed as such.

## 1. Credited

- **256 strict full-C game routines** (230 overlay, 26 resident) and **302 verified
  identities** (275 overlay, 27 resident), from `function_accounting_checkpoint.json`
  (queue originals 898, unknown ownership 507). Both whole-game denominators are unknown.
  The 815-body figure is a ceiling on game-tier functions, not a denominator (task t6).
- In the U38/U3B0/U25 work this lane touched, now credited: `OVL_0038_3134`,
  `OVL_0038_101D`, `OVL_0038_2E08`, `OVL_03B0_11F3`, `OVL_0025_1CC3`, `OVL_0038_12B2`
  (accepted after the first version of this page, #1224 via #1232), and the installer
  `OVL_0038_31A2`. Tonight (§1a): `OVL_03B0_0C33`, `OVL_03B0_0A08`, `OVL_03B0_07A4` and
  `OVL_03B0_01DA`. Read status from the CSVs, never from prose.

## 1a. Tonight, 2026-10-20 evening: what moved and why

**Credits 252 -> 256, identities 298 -> 302. All four were verdicts, not translations.** Each body had been merged evidence-only for days: a full 1 MiB differential with the EGA aperture, native corroboration and a registered checker. Not one byte of C changed tonight. What changed was an identity decision under the two-factor standard (par-a, i20050; the argument is in each evidence doc via #1262). par-e then landed identity, strict, accounting and pin together, one commit per row.

| Credit | Bytes | Factor 1 (uncredited `game_evidenced` caller, decoded `push cs; call`) | Factor 2 (documented subject) |
| --- | ---: | --- | --- |
| `OVL_03B0_0C33` (253) | 146 | 4 sites in `0D9A` | parent-frame slots and `DS:6B29` defined by credited 0CC5's doc |
| `OVL_03B0_0A08` (254) | 251 | 1 site in `0D9A` | string-node flag +29h, `DS:6B29` |
| `OVL_03B0_07A4` (255) | 307 | 2 sites (`0C33`, `0D9A`) | the 41-byte string node (+29h flag, +2Ah/+2Ch links), as for 11F3 |
| `OVL_03B0_01DA` (256) | 419 | 4 sites in `03BD` | 03BD's copy of the menu key list (GAMEPLAY_PATH §2.4). **Limit: only the main-menu caller is documented** |

`reviewed_commit` for all four is `5b225a62`, form 2 of the [definition](REVIEW_LANE_HANDOFF.md#reviewed_commit-definition-vp-i20093-2026-10-20).

**Refused: `OVL_03B0_06FB`.** Its evidence is as good as the other four, and it passes factor 1 easily (19 fixup-backed sites). But its 13 instructions have no memory operand: it pushes constants and calls resident `0709:05EB`. Its subject is a display service, which is not game state. It stays evidence-only. The refusal is what makes the four credible: the same standard, applied the same night, said no.

**Readiness: 447 candidates / 58 composable -> 443 / 54** (export on `3a2b1bd0`).
- The whole move is the four credits leaving the candidate set.
- The two `driver_internal` members (`RES_08AE_181A`, `RES_08AE_0000`) each have a readiness delta of **zero**, measured with and without each member.
- These are readiness numbers. They must not be added to, or read into, the credit count.
- The honest pool is now **10** composable rows with ownership evidence. The cheapest is `OVL_006A_0824` at 155 B. **Cheapest is not easiest.** 0824 has no fixup-backed caller. Its single known call site is one aligned `push cs; call` at `+0A4E`, inside `OVL_006A_090A`, which is itself uncredited, `game_evidenced` and provisional. ds4 flagged that it needs a genuine reached-caller capture before it can be credited: evidence that the site is actually reached with the game state factor 2 would cite. That is a night of work, not a verdict like tonight's four. The other 44 are 34 callerless 5-byte tails, 3 unresolved rows (including the refused 06FB) and 7 helper candidates.

**Why freshness is now enforced rather than hoped for.** Tonight three numbers were true of a file and false of the tree:
- **453** candidate rows: par-b's export, written at 12:22:50;
- **455** rows: ds4's non-terminator bucket;
- **578 B** instead of **252 B** for a row whose callee `OVL_006C_3291` was labelled "untranslated" though it had been credited at 12:47.

Each came from an export that was correct when written; nothing was wrong with how the labels are derived. The defect was that existence was treated as freshness: `price_candidates.py` regenerated only a *missing* export, and the other consumers read whatever file they were given. Even the reviewer's own 20:31 export was stale an hour later. Since #1272, every consumer calls `strict_candidate_queue.load_fresh_export`. It compares the export's recorded hashes of the six committed input tables with the tree and exits 2 on any difference, naming each changed input and printing the regeneration command. **Quote a queue count with its commit or timestamp, or not at all.**

## 2. Evidence at the bar, **not credited** (author evidence only)

| Row | PR | Cases / coverage | Status |
| --- | --- | --- | --- |
| `OVL_03B0_07A4` | #1202 | 105; caller 115/115, 8/8 | **credited 2026-10-20 (255)**, §1a |
| `OVL_03B0_0A08` | #1204 | 202; 107/107, 2/2 | **credited 2026-10-20 (254)**, §1a |
| `OVL_03B0_0C33` | #1209 | 118; 49/49, 6/6 | **credited 2026-10-20 (253)**, §1a |
| `OVL_03B0_06FB` | #1210 | 63; 13/13 | evidence-only; **identity refused** (factor 2, §1a); has a manifest `full` row (readiness only) |
| `OVL_03B0_01DA` | #1216 | 95; 171/171 feasible starts, 15/15 outcomes; extent repaired 483 to 419 B | **credited 2026-10-20 (256)**, §1a |
| `RES_0464_248E` | par-b branch | 41 cases; all figures reproduced | not gated, branch cut from old main |
| `OVL_03B0_1140` | n/a | identity decision (b): its list head dies in stack cells | caveat: I read 0D9A's stores in bytes, did not run it |

The four U3B0 rows marked credited above are now in both CSVs. The others are in neither. `OVL_03B0_06FB` is the one with a manifest
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
| `OVL_0077_299C` | >= 1,011 (>= 1,795 with 0D9A) | 7,610 / 17 | yes | needs 008A, 02E8; also reaches the unpriced far target `0428:002A` through `RES_03F1_0000` |
| `OVL_006F_0010` | >= 627 (one-hop; it counted 5 endpoints) | 956 / 6 (**11** unpriced library endpoints: 9 uncovered SYSTEM, `0A32:000B` Intr via `RES_0899_0000`, and `0A32:00A2` behind the 08AE set) | yes | the endpoint count to use is the closure's 11, not the one-hop 5; see `DRIVER_INTERNAL_CLASS.md` |
| `OVL_0025_0511` | >= 303 | 8,120 / 28 | yes | cheapest by one-hop floor, **not** buyable by translation alone |
| `OVL_0025_0411` | >= 926 | 3,148 / 17 | yes | |
| `OVL_0025_1684` | >= 1,499 | 5,908 / 14 | yes | |
| `OVL_03CE_01C8` | >= 1,990 | 6,898 / 21 | yes | |
| `OVL_006F_22D5` | >= 2,404 | 8,989 / 21 | yes | |
| `OVL_009E_0026` | >= 38 + dispatch closure | 314 / 1 | no | dispatch through itself (`indirect_unresolved`) |
| `OVL_005E_0123` | >= 2,029 | 2,549 / 3 | no | dispatch via `OVL_009E_0160`; 009E_02FA is on hold (i19596) |

The far targets outside SYSTEM, the resident segments and 08AE (`0A32:000B`, `0A32:00A2`, `0428:002A`) were found by a tool column that did not exist when this summary was written: before #1244, `price_row_closures.py` collected them and never printed them. That is why the 0010 and 299C entries changed after par-a's re-verification (i19957); the byte and body figures did not change.

**Pricing conclusion.** Every priced row except `009E_0026` and `005E_0123` has
`08AE:00DC` and `08AE:017B` in its closure, reached through the resident gateways
`RES_0899_0000` and `RES_0899_0059`. That includes `0511`, which the one-hop table ranks
cheapest. Under the 08AE policy (§4) those rows cannot be credited by translation alone;
the two that are 08AE-free are blocked by indirect dispatch and a held callee.

**Bought blocker (VP i20006).** The two `0899` gateways (222 B: `0059` ReadKey-with-hotkeys and `0000` QUIT) are funded as their own task. They are **necessary** for 303 of 447 candidate rows, and **sufficient for one** (`OVL_007F_0294`); par-a regenerated both counts on `b3fd5718`. `0000` ends in Halt, so every path through it is a process exit. Numbers, costs and the cheapest enabled rows are in [IDENTITY_CLEARED_PRICING.md](IDENTITY_CLEARED_PRICING.md#bought-blocker-the-0899-gateways-vp-i20006-2026-10-20).

**"58 composable" is not 58 credits (VP i20035; measured by par-a on main `b3fd5718`).** *(Superseded by §1a after tonight's four credits: 443 / 54, with 10 owned rows. Kept as measured at `b3fd5718`.)* Readiness is not reachability, and neither is ownership. The strict-candidate export has 447 candidates; strict-credited rows are already excluded from it. **58** have zero unready callees, and all 58 are flagged `composable`:
- **34** are 5-byte tails: `callers.points = 0`, no fixup-backed caller, all `helper_candidate`, and prior status `source_mention` (30), `inline_in_model` (3) or `inline_in_strict` (1). They are unreachable fragments, not work.
- **24** remain:
  - **13** with game ownership evidence (`game_evidenced` / `game_verified`): `03B0_0C33` 146 B, `006A_0824` 155, `001C_2E7D` 176, `0077_17ED` 176, `007D_05F0` 232, `03B0_0A08` 251, `0023_0239` 296, `03B0_07A4` 307, `0020_11AC` 316, `0068_0294` 587, `0025_07BC` 696, `0038_0CC3` 745 (no callers), `0033_08A4` 755;
  - **4** `game_or_other_library_unresolved`: `03B0_06FB` 22, `007F_0000` 32, `0033_0B97` 71, `03B0_01DA` 419;
  - **7** `helper_candidate`: `03D2_00F7` 5 (one fixup caller), `007F_0020` 26, `03C2_0000` 46, `03BD_031F` 49, `03C6_0000` 104, `03B0_1140` 179, `03C2_02CC` 342.

So the honest pool is **13 rows with ownership evidence, or 17 counting the unresolved four**, not 58. The cheapest ownership-evidenced row is **`OVL_03B0_0C33` at 146 B**. The 22, 26 and 32 B rows are ownership-unresolved or helper candidates, not cheap credits. There is no cheap credit left in the queue; remaining work starts at 146 B. (The first figures quoted, 63/7/56/40/21, were the ds4 lane's own ownership filter enumerated over the same export, not a product. Prefer the measured 34/24/13 above; a lane's arithmetic is not a regenerated product, VP i20043.)

**Proposed identity rows: four U3B0 rows (par-a, VP i20053; arguments in each evidence doc via #1262).** These are proposals in text. par-e lands them as one PR with four commits, in this order (cheapest first). Each commit lands identity, strict, accounting and the pin together. Columns follow `reviewed_function_identities.csv`. Each body hash was recomputed from GAME.OVR. **Landed 2026-10-20 as strict 253 to 256.** The landed rows carry `reviewed_commit` `5b225a62` (the #1262 commit that holds the identity arguments), not the author heads proposed below. That is form 2 of the [definition in REVIEW_LANE_HANDOFF.md](REVIEW_LANE_HANDOFF.md#reviewed_commit-definition-vp-i20093-2026-10-20), and the author heads are kept here only as the proposal.

Factor 1 for all four rests on `game_evidenced` callers that are **not themselves credited**. That is the same basis as `OVL_03B0_11F3`: the two-factor standard accepts a call site inside a `game_evidenced` function, so the identity does not come from a chain of credits.

| # | id | evidence_doc | review_pr | author head (proposed; landed as `5b225a62`) | body_sha256 | body_bytes | Factor 1 (decoded `0E E8` near sites, inside the caller's extent) | Factor 2 |
| --- | --- | --- | ---: | --- | --- | ---: | --- | --- |
| 1 | `OVL_03B0_0C33` | `docs/re/U3B0_ENTRY_0C33_CPU.md` | 1209 | `7df3d1ea877f847844fd1e5031922168f1fbad19` | `c7cb3ab8625088a43a7c7d35f9fa21dde2252bdfa9132e2fc5592070489fe86a` | 146 | 4 in uncredited `game_evidenced` `0D9A`: `+1023`, `+1038`, `+1049`, `+1057` | the parent-frame slots and `G=DS:6B29` that credited 0CC5's doc defines |
| 2 | `OVL_03B0_0A08` | `docs/re/U3B0_ENTRY_0A08_CPU.md` | 1204 | `1e7c1d7d897951e880a0da9f93b93c2fc999b5d1` | `1b6c5b1ac9f26c64572151759cc73a44003c4321870f36d907e4f3d7d32bbf86` | 251 | 1 in uncredited `game_evidenced` `0D9A`: `+0FEB` | node flag +29h (U86_STRING_NODE_028D), `DS:6B29`, list head defined in 0CC5's doc |
| 3 | `OVL_03B0_07A4` | `docs/re/U3B0_ENTRY_07A4_CPU.md` | 1202 | `7628de350784a330ddb471edd13cdd96d35b86ab` | `b4d0d09d692e50b31125fa64fbedb3316385f9dd60dc1efb5c74605b2e669eef` | 307 | 2 in uncredited `game_evidenced` callers: `+0CBC` in `0C33`, `+0EE4` in `0D9A` | the 41-byte string node: +29h flag, +2Ah/+2Ch links (same basis as 11F3) |
| 4 | `OVL_03B0_01DA` | `docs/re/U3B0_ENTRY_01DA_CPU.md` | 1216 | `db3caa0821377cf239b1c4b978a17cf4a9796ea5` | `8676c97ae6dcaa88c9f7d68d75c75f81e4936572b517bd84a7e19876c58d96ce` | 419 | 4 in uncredited `game_evidenced` `03BD`: `+048B`, `+05E2`, `+0606`, `+0685` | 03BD's copy of the menu key list, traced from the media (GAMEPLAY_PATH §2.4). **Limit: only the main-menu caller of 03BD is documented; the strings of its other 34 callers are not claimed.** |

**Refused:** `OVL_03B0_06FB` fails factor 2. Its 13 instructions have no memory operand; it pushes constants and calls resident `0709:05EB`. It stays evidence-only, and its manifest `full` row stays a readiness declaration.

**Ceiling:** if all four land, the count goes from 252 to at most **256**.

**Ordering note:** row 1 (`0C33`) near-calls row 3 (`07A4`), which is already `modeled`. The export marks `0C33` composable today, so landing it before `07A4` needs no readiness change.

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
