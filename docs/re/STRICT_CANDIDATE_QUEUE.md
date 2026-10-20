# Strict full-C candidate queue

`tools/overlay/strict_candidate_queue.py` ranks the **non-strict overlay
originals** as strict full-C work candidates and records, for each one, any
prior translation found in the main tree, inlined bodies, unmerged branches or
commit messages. It answers "what should the next strict translation be, and
what has already been attempted?" — not "how complete is the game?". The
whole-game denominator stays open and no completion percentage is calculated.

## Schema 9 metadata repair

[QUEUE_METADATA_REPAIR.md](QUEUE_METADATA_REPAIR.md) records the original-byte
proof and regenerated rows. U86 +3092 is 5 bytes / 4 instructions, and U6A
+0824 is 155 bytes / 57 instructions; hash-checked corrections apply at ledger
generation. U9E +0000 now reports an explicit `indirect_unresolved` target
at +001E and is not composable. The blocker propagates to modeled callers.
SYSTEM +0C10 is covered by `pr_system_strcmp_0c10`; U25 +1815 still has a
semantic-only API and remains interface-caveated.

On the unchanged input identities, the schema-9 export (re-derived on the
244/290 main after #1162, queue SHA-256
`9b04734f31cf8aa367769fa5245e8c8fffffad66a3194c1a20a8127b01f169eb`) has 455
candidates, 63 composable rows (72 without the caveats), nine interface-caveat
rows, and 14 rows that explicitly name an unresolved indirect target (the
dispatcher and 13 direct callers).
These are queue metadata, not credit or completion counts. The schema-8
ranked tables, hashes and historical traps below remain dated snapshots;
they are not a fresh authoring backlog.

## Method

Inputs are the regenerated hash-pinned products (`inventory.json`,
`callgraph.json`, the decoded near-call audit `near.json`) plus the checked-in
[function ledger](overlay_function_ledger.csv),
[classification](overlay_classification.csv),
[strict full-C manifest](strict_full_c_addresses.csv) and a read-only view of
git history. The join reuses `function_queue.overlay_rows` and refuses
inconsistent inputs. `START.EXE`/`GAME.OVR` hashes are checked before use and
the near-call audit is only accepted when its failure counts are zero and its
site totals match the ledger.

**Candidate set.** Every overlay original in the inventory join (673) minus
the 148 overlay addresses credited in the strict manifest; the ten resident
credits are counted and ignored (this tool is overlay-only).

**Callee readiness.** From callgraph edges whose `src_func` is the candidate:
decoded-validated near calls with exact prologue targets and fixup-confirmed
far calls. The callgraph's near scan is a raw byte scan, so phantom `E8`
candidates inside instruction bytes (for example the byte inside a `SHR AL,1`
encoding) are filtered by requiring the exact-prologue flag. Each overlay
target is `strict` (strict manifest), `partial` (manifest `partial`: bounded
or provisional, and it takes precedence over a mapped file), `modeled` (a
merged per-routine file in `src/recovered`, `include/poolrad/recovered` or
`tests/recovered`, or a manifest `full` entry), `modeled_blocked` (a merged
model whose transitive closure still contains an unready callee), `verified`
(classification `game_verified` with no model) or `untranslated`. A strict
overlay credit is `interface_caveat` instead of `strict` when its published
C endpoint is not a raw-CPU callee (for example
`OVL_0025_1815`/`+18EB` expose `ecl_helpers.h` semantic compare APIs).
The string comparator's runtime dependency `0AF8:0C10` already has the
merged `pr_system_strcmp_0c10` raw model, but the overlay API still needs
CPU/stack effects before a caller can pass it a live CPU and RAM
image. Resident
targets are `resident_covered` only when their `RES_<seg>_<off>` id is both
strict-credited and listed `full` in the callee-model manifest (the
callbacks only read and write the same RAM, as with `RES_00BA_2E8B`
composed through `+101D`'s RAM-backed IO adapter), `resident_callback` when
the manifest lists the credit as `callback` (true external effects such as
release or presentation are left to a host callback),
`resident_interface_caveat` when a `full` credit's published endpoint exposes
callback memory I/O rather than a CPU-state callee (`RES_00BA_23DB` through
`resident_party_count.h`; `RES_00BA_2199` gained the raw-CPU
`pr_res_00ba_2199_cpu` model in PR #902 and is no longer caveated), else
`resident_unmapped` — an unlisted strict credit counts as unready so a
future credit cannot slip through. Runtime targets are `runtime_covered`
only for a target in the checked-in `RUNTIME_COVERED_ENDPOINTS` set, which
lists the SYSTEM/runtime leaves with merged raw-CPU full-state models
(`0AF8:0A3F` Move, `0AF8:0B25` StrStore, `0AF8:0DC0` set IN, `0AF8:11C0`
RANDOM, `0AF8:11D6` random bit, `0AF8:120F` seed step, `0AF8:16A8`
FillChar, `0AF8:182A` UpCase), else `runtime_unmapped`.
**Only strict, closure-ready modeled, resident_covered and runtime_covered
count as ready; `indirect_unresolved`, `interface_caveat`, `resident_interface_caveat`, `partial`,
`modeled_blocked`, `verified`, `resident_callback`, `resident_unmapped` and
`runtime_unmapped` do not**: readiness reports `ready`, `unready` and
`total`, and a body is `composable` only when every target is ready. Readiness is the
transitive closure over overlay, resident and runtime edges: strict bodies
and manifest-`full` models are reviewed endpoints that handle their own
environment calls; every other modeled body is ready only when each overlay
callee in its closure is ready, each resident target is strict-credited and
no runtime call is uncovered. The schema-2–5 corrections (2026-10-17, after
Claude's, Astra's, Codex's and the #848 review) stop the queue proposing
bodies whose callees are not yet full-state composable.

**Unresolved indirect dispatch.** Direct-call graphs omit dynamic targets.
The evidence-backed `UNRESOLVED_INDIRECT_DISPATCH` map records known open
boundaries. A dispatcher receives a target with `kind="indirect"`,
`readiness="indirect_unresolved"`, a known instruction `site`, and null
target `seg`/`offset` because the live handler address is unresolved.
An overlay edge to that dispatcher also carries the reason and evidence
document. Explicit dispatch blockers override a stale strict/full label,
and propagate through modeled callers. This is not an exhaustive audit
of every indirect call in the corpus.

**Caller evidence.** Fixup-confirmed far callers resolving to the candidate's
exact start, and decoded near callers with an exact prologue target. An
`aligned PUSH CS` caller is one whose call-site opcode is immediately preceded
by `PUSH CS` (`0x0E`) in the hash-pinned `GAME.OVR` — the Turbo Pascal overlay
call convention documented at sites such as `push cs; call +0x07BC`.

**Prior-translation detection.**

* Main tree: a candidate with its own per-routine file is `main_model`. A
  mention in a source file whose routine is strict-credited is
  `inline_in_strict` (the +07BC-in-+0E0C case); in a merged model,
  `inline_in_model`; anywhere else in source, tools or docs, `source_mention`.
* Address tokens: explicit `OVL_XX_YYYY`, `0xXX:0xYYYY` and `uXX_YYYY` forms
  count anywhere; bare `+0xYYYY` / `+YYYY` / `0xYYYY` forms require unit
  context on the same line or in the file name, so offsets inside unrelated
  hex strings do not match. File names accept both repository conventions
  (`uXX_YYYY_words` and `uXX_words_YYYY`).
* Branches: file names under the five evidence directories on every
  **unmerged** tip (a tip whose commit is not an ancestor of `origin/main`).
  Merged tips are main-tree state and are skipped.
* Commits: all-reachable subjects and bodies, same token rules.

**Scoring.** `score = ownership + grade + callee + callers + size + composable
- penalty`: ownership 40/25/5/0 for game_verified/game_evidenced/
helper_candidate/unresolved; grade 20/10/5/0 for A/B/C/D; callee
`round(min(30, 6·strict + 4·modeled + 2·verified + 1·partial +
1·modeled_blocked + 2·resident_covered + 1·runtime_covered) · ready/total)`
(12 for a leaf, `ready` = strict + modeled + resident_covered +
runtime_covered), so unready targets dilute the callee points; callers
`min(15, 5·fixup) + min(10, 2·aligned PUSH CS) + 2·(near)`; size 12/8/4/2/0
for ≤48/≤128/≤256/≤512/larger bytes; `composable` +6 when every callee is
ready; prior penalty 45 main_model, 40 inline_in_strict, 25 unmerged_branch,
18 inline_in_model, 8 commit_mention, 5 source_mention, +5 per additional
strong channel, capped at 60. Ties break on ID. Every row carries its
`score_terms`.

**Determinism and pins.** Sorted inputs everywhere; with frozen refs two
identical runs produce byte-identical JSON. The export records SHA-256 for
every input file, the branch-tip table, and the raw `git log` scan. The
schema-9 export (queue SHA-256
`9b04734f31cf8aa367769fa5245e8c8fffffad66a3194c1a20a8127b01f169eb`,
on the 244/290 main after #885–#1162 plus the phantom-edge filter, the
inline-prior filter, the remaining raw-CPU interface caveats and the
modeled-system-runtime coverage) was generated from `START.EXE` `d825df2b…a07bd`,
`GAME.OVR` `872964a3…28614`, inventory `6de7664f…ca7f8`, callgraph
`c180ff95…f4dc3`, near audit `a40ba4f2…cbd2c`, with 1714 refs / 1207 tips /
537 unmerged tips (`tips_sha256` `0668410a…`) and 2664 commits (`commits_sha256`
`160aca38…`). Git pins are point-in-time;
other lanes keep pushing refs. **The digest is point-in-time at the recorded
commit: it does not track HEAD, and a branch-ref advance alone does not require
a re-pin — see the checkpoint for the pin used for credit.**

## First ranked list (top 25 of 457)

| rank | id | unit:offset | bytes | insns | grade | classification | callees (strict/modeled/verified/untranslated) | ready/unready | composable | fixup callers | aligned PUSH CS | prior | score |
| ---: | --- | --- | ---: | ---: | --- | --- | --- | --- | --- | ---: | ---: | --- | ---: |
| 1 | `OVL_03B4_016D` | `0x3b4:0x016d` | 307 | 114 | A | game_evidenced | 5/0/0/3 | 5/3 | no | 10 | 0 | source_mention | 76 |
| 2 | `OVL_001C_0292` | `0x1c:0x0292` | 78 | 30 | B | game_verified | 4/0/0/0 | 4/1 | no | 0 | 1 | commit_mention | 74 |
| 3 | `OVL_0077_004A` | `0x77:0x004a` | 1219 | 495 | A | game_evidenced | 1/0/0/3 | 8/7 | no | 3 | 3 | source_mention | 72 |
| 4 | `OVL_001C_0EBC` | `0x1c:0x0ebc` | 76 | 29 | B | game_verified | 3/0/0/0 | 3/1 | no | 0 | 1 | source_mention | 71 |
| 5 | `OVL_001C_19B0` | `0x1c:0x19b0` | 1025 | 405 | B | game_verified | 4/0/0/0 | 11/4 | no | 0 | 1 | source_mention | 71 |
| 6 | `OVL_009E_0D63` | `0x9e:0x0d63` | 128 | 53 | A | game_evidenced | 1/0/0/0 | 1/1 | no | 14 | 1 | source_mention | 71 |
| 7 | `OVL_009E_1656` | `0x9e:0x1656` | 263 | 109 | A | game_evidenced | 1/0/0/0 | 5/4 | no | 7 | 2 | source_mention | 71 |
| 8 | `OVL_0086_331A` | `0x86:0x331a` | 249 | 113 | A | game_evidenced | 0/0/0/0 | 8/1 | no | 2 | 1 | source_mention | 70 |
| 9 | `OVL_009E_1869` | `0x9e:0x1869` | 201 | 75 | A | game_evidenced | 3/0/0/0 | 4/3 | no | 4 | 0 | source_mention | 70 |
| 10 | `OVL_03BD_0E08` | `0x3bd:0x0e08` | 703 | 278 | A | game_verified | 0/0/0/1 | 6/7 | no | 8 | 0 | commit_mention | 70 |
| 11 | `OVL_001C_0BD1` | `0x1c:0x0bd1` | 163 | 66 | B | game_verified | 3/0/0/1 | 6/2 | no | 0 | 1 | source_mention | 69 |
| 12 | `OVL_001C_1EF8` | `0x1c:0x1ef8` | 1747 | 653 | B | game_verified | 5/0/0/1 | 8/4 | no | 0 | 1 | source_mention | 69 |
| 13 | `OVL_001C_25CB` | `0x1c:0x25cb` | 159 | 66 | B | game_verified | 3/0/0/0 | 5/2 | no | 0 | 1 | source_mention | 69 |
| 14 | `OVL_005E_0D23` | `0x5e:0x0d23` | 303 | 102 | A | game_evidenced | 1/0/0/0 | 3/1 | no | 3 | 1 | source_mention | 69 |
| 15 | `OVL_03C6_02D5` | `0x3c6:0x02d5` | 203 | 87 | A | game_evidenced | 2/0/0/2 | 2/2 | no | 6 | 1 | source_mention | 69 |
| 16 | `OVL_001C_2CCF` | `0x1c:0x2ccf` | 260 | 88 | B | game_verified | 5/0/0/2 | 5/4 | no | 0 | 1 | source_mention | 68 |
| 17 | `OVL_0086_0C20` | `0x86:0x0c20` | 815 | 322 | A | game_evidenced | 4/0/0/4 | 8/10 | no | 6 | 0 | source_mention | 68 |
| 18 | `OVL_001C_1435` | `0x1c:0x1435` | 500 | 177 | B | game_verified | 3/0/0/1 | 4/1 | no | 0 | 1 | source_mention | 67 |
| 19 | `OVL_006C_3291` | `0x6c:0x3291` | 342 | 100 | A | game_evidenced | 1/0/0/1 | 1/1 | no | 7 | 2 | source_mention | 66 |
| 20 | `OVL_009E_0AE7` | `0x9e:0x0ae7` | 360 | 164 | A | game_evidenced | 2/0/0/2 | 4/3 | no | 4 | 0 | source_mention | 66 |
| 21 | `OVL_009E_0F06` | `0x9e:0x0f06` | 204 | 74 | A | game_evidenced | 2/0/0/2 | 4/5 | no | 3 | 0 | source_mention | 66 |
| 22 | `OVL_03BD_10C7` | `0x3bd:0x10c7` | 319 | 124 | A | game_verified | 0/0/0/0 | 5/6 | no | 2 | 0 | commit_mention | 66 |
| 23 | `OVL_005E_08B2` | `0x5e:0x08b2` | 952 | 344 | A | game_evidenced | 4/0/0/2 | 6/5 | no | 2 | 0 | source_mention | 65 |
| 24 | `OVL_007D_0C9A` | `0x7d:0x0c9a` | 454 | 181 | A | game_evidenced | 2/2/0/6 | 10/7 | no | 1 | 0 | source_mention | 65 |
| 25 | `OVL_007F_0CEB` | `0x7f:0x0ceb` | 659 | 260 | A | game_evidenced | 1/1/0/3 | 7/4 | no | 4 | 0 | source_mention | 65 |

Queue summary (schema 9, on the 244/290 main after #885–#1162, the
phantom-edge and inline-prior filters, the remaining raw-CPU interface caveats
and modeled system-runtime coverage): 457 candidates, 72 would be composable without the
caveats and 63 are composable — 9 bodies carry an interface caveat;
classification game_evidenced 301, game_verified 45, helper_candidate 45,
unresolved 66; grades A 278, B 177, D 2; prior status main_model 73,
inline_in_model 20, inline_in_strict 20, unmerged_branch 9, commit_mention 25,
source_mention 310; 603 fixup-backed and 624 aligned-PUSH-CS caller sites across
the queue. Near-call evidence is `decoded_validated` for this export.


## Highest-leverage next recoveries

A callee that is the *only* untranslated dependency of a candidate row gates that
row outright. The **fan-out** (rows gated) is not the **cost**: several top rows
have deep dependency trees, so they are not cheap wins. `transitive cost` sums
every transitive untranslated dependency's own size (the blocker plus its
closure); `leaves` and `depth` describe that closure. Costs use each
candidate's bounded-decode size, so they are a lower bound on recovery effort.

| single untranslated callee | rows it alone gates | of those game_evidenced | blocker own B / insns | transitive cost B / insns | transitive leaves | depth |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `OVL_0086_08C3` | 32 | 28 | 462 / 177 | 6257 / 2541 | 11 | 7 |
| `OVL_03B0_03BD` | 11 | 11 | 830 / 298 | 910 / 329 | 1 | 2 |
| `OVL_009E_1344` | 6 | 5 | 786 / 301 | 5176 / 2112 | 8 | 6 |
| `OVL_03B0_10D1` | 5 | 5 | 111 / 52 | 1021 / 381 | 1 | 3 |
| `OVL_009E_175D` | 5 | 5 | 268 / 94 | 268 / 94 | 1 | 1 |
| `OVL_03B0_0D9A` | 5 | 4 | 823 / 333 | 1733 / 662 | 1 | 3 |
| `OVL_009E_115C` | 4 | 4 | 488 / 189 | 488 / 189 | 1 | 1 |
| `OVL_0038_0058` | 4 | 4 | 181 / 62 | 4571 / 1873 | 8 | 6 |
| `OVL_0038_1545` | 4 | 0 | 166 / 67 | 4865 / 1993 | 9 | 7 |
| `OVL_0025_054E` | 3 | 0 | 353 / 124 | 5899 / 2393 | 8 | 5 |
| `OVL_005E_1884` | 3 | 2 | 666 / 226 | 12433 / 4996 | 21 | 8 |
| `OVL_006C_3291` | 3 | 2 | 342 / 100 | 464 / 159 | 1 | 2 |

Read it as: a large fan-out with a deep, expensive closure must not be read as a
cheap multi-row unlock; prefer blockers whose transitive cost and depth are
small.

`OVL_0025_1815` is additionally gated on the unmodeled runtime string compare
`0AF8:0C10` (not a `RUNTIME_COVERED_ENDPOINTS` member), and U38
`+2556/+070F/+0C4B` and `+0B1B/+2610` remain gated on `RES_00BA_16EC` and
`OVL_0038_0058` respectively even after the SYSTEM string leaves `0B0B/0B98`
became covered.

## Claude's traps, as detected

| trap | queue evidence |
| --- | --- |
| U25 `+07BC` inlined in strict `+0E0C` | `inline_in_strict`, covered by `OVL_0025_0E0C`; mentions at `src/recovered/overlay_u25_load_word_0e0c.c:1,405`; its one near caller is the documented `+0E4A` aligned `PUSH CS` call; rank 541, score 11 |
| U33 `+0B97` | `unmerged_branch`: `src/recovered/overlay_u33_pair_writer_0b97.c` and docs on `codex/fullc-u33-0b97` |
| U33 `+0221` | `unmerged_branch`: `overlay_u33_clipped_writer_0221.c` on `codex/fullc-u33-0221` |
| U33 `+1403` | `unmerged_branch`: `overlay_u33_both_outside_1403.c` on `codex/u33-1403-retf-wrap` |
| U6C `+3FB3` | `unmerged_branch` on `codex/u6c-record-tier-3fb3` plus commit `010074dc` ("Compose complete U6C +3FB3 caller…"); callee `+3F31` strict |
| U6C `+408C` | `unmerged_branch` on `codex/u6c-record-weighted-408c` plus commit `ed314c24`; callee `+3F31` strict |
| U38 `+12B2` | `unmerged_branch`: `overlay_u38_effect_match_12b2.c` on `codex/fullc-u38-12b2`; callee `+0C06` strict |
| U38 `+25BC` / `+264B` / `+2A87` / `+2D88` | callee `OVL_0038_101D` readiness **modeled** — the merged provisional target-selector helper that composes the resident selector through its hook; each also has an unmerged attempt on its `codex/fullc-u38-*` branch, reported separately |

The `+101D` helper itself appears as `main_model` (rank 598): its merged
provisional full-C model is why the four callers above read as callee-ready.

Readiness corrections (schemas 2–5, 2026-10-17): resident, runtime, partial,
verified-without-model and closure-blocked callees all count as unready —
e.g. `OVL_0033_1E31` (3/14), the U9E dice candidates, the U1C
`+2C03`/`+2F2D`/`+3171`/`+0C77` cases whose verified or blocked callees
previously slipped through, and `OVL_0025_0000`, which is composable again
now that #862/#863 made its `+1153`/`+12FB` string cores full models.
Schema 5 (the #848 review) closes resident and runtime edges too: the six
U38 rows `+25BC`/`+264B`/`+2A87`/`+2B7D`/`+2D88`/`+2AE6` are composable
because their `OVL_0038_101D` callee's resident selector `RES_00BA_2E8B` is
strict-credited and manifest-`full` (`resident_covered`), not because the
edge was ignored. Schema 6 classifies all nine resident credits: the two
sidecar models `RES_00BA_1509` (release/copy/presentation callbacks) and
`RES_00BA_15F7` (allocation callback) are `callback`, so `OVL_0025_1DF0`
reads `resident_callback` and drops out of composable. A follow-up filters
phantom near-call candidates from the raw byte scan by requiring the
exact-prologue flag, so rows such as `OVL_0033_08A4` no longer read unready
because of an `E8` byte inside another instruction. Composable bodies carry
a +6 score term. Schema 7 (queue hygiene after #890) stops counting strict credits whose
published C endpoint is not a raw-CPU callee: `OVL_0068_0109`
(`overlay_u68_map_update.h` scalar arguments and a segment view) and resident
`RES_00BA_23DB` (`resident_party_count.h` memory callbacks). The VP's
`RES_00BA_2199` caveat (`resident_sidecar_lookup.h` callback memory I/O) was
removed once PR #902 landed the raw-CPU `pr_res_00ba_2199_cpu` full-state
model, restoring U31 `+0675` and U25 `+1FB1` to composable. Nine bodies now
carry an interface caveat and three (`OVL_0068_04DF`, `OVL_0068_0294`,
`OVL_0038_0400`) drop out; the other six were already unready for other
reasons. On the 235/281 main (214 overlay credits) the schema-9 candidate set is 459 and 57 bodies are composable.

`OVL_0068_0294` is the last row held back from this pool. It is recorded as a
disposition, not an acceptance: bounded model, no identity, no strict credit,
with its `AF` exception and unmeasured closure gaps named. See
[OVL_0068_0294_DISPOSITION.md](OVL_0068_0294_DISPOSITION.md).

## Reproduce

Synthetic checks need only Python:

```sh
python3 -m unittest tools/overlay/test_strict_candidate_queue.py
```

With Capstone 5.0.7 available, regenerate the hash-pinned products first
(the audit re-checks the three checked-in ledgers byte-for-byte and requires
byte-identical repeated queue exports):

```sh
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/function_queue_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --out-dir resources/function-accounting-current
```

Then export the strict candidate queue (`git fetch origin` first so the
branch scan sees current tips):

```sh
python3 tools/overlay/strict_candidate_queue.py \
  --inventory resources/function-accounting-current/inventory.json \
  --callgraph resources/function-accounting-current/callgraph.json \
  --near-calls resources/function-accounting-current/near.json \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --repo . \
  --json resources/strict-candidate-queue/queue.json
```

Local JSON products stay under ignored `resources/`; the export contains
addresses, file names, counts and short quoted repository lines only. Owner
media are read without modification. `--no-git` skips the branch/commit scans
for a fast overlay-only pass.

## Limits

* Mention scanning is heuristic. Docs mention many addresses, so 397 of 600
  rows carry at least one low-weight `source_mention`; the strong signals are
  own files, strict-model inlines, unmerged branch files and commit matches.
* File-name routine extraction cannot map files that omit the offset (for
  example `overlay_u3bd_wallset.c`); such files contribute no `main_model`
  evidence.
* Commit-message detection requires an explicit ID or an offset with unit
  context; branch file names remain the primary unmerged-translation detector.
* Resident targets are mapped to `RES_<seg>_<off>` ids from the callgraph's
  runtime stub address and are ready only when that id is strict-credited
  AND manifest-`full`; `callback` and unlisted credits are unready. Runtime
  targets (the SYSTEM segment) have no per-target id and are ready only for
  reviewed endpoints, so an uncovered runtime call blocks every other
  modeled body.
* The frozen queue SHA-256 is point-in-time: it hashes the branch-tip table
  and raw `git log` scan, so re-running at a later ref changes the digest even
  when nothing else does. The stable invariants are the counts (candidates,
  composable, interface-caveat bodies) and the classification/grade tables;
  compare those, not the digest, when re-deriving at a new ref.
* Near call sites remain candidate-based: the decoded audit validates
  instruction boundaries under the prologue-seed assumption, not the seeds
  themselves.
* [partial_callee_models.csv](partial_callee_models.csv) is the curated
  callee-model manifest (`id,status,evidence_doc`; overlay statuses `full`
  and `partial`, resident statuses `full` and `callback`). It carries models
  whose shared file name cannot be mapped by filename (the U25 string cores
  `OVL_0025_1153`/`OVL_0025_12FB` are `full` since #862) and classifies all
  nine strict resident credits (schema 6; seven `full`, the sidecar
  remove/append `callback`), and must be updated when a model's completeness
  changes.
* Ledger `bound`/`decoded` sizes can exceed the actual callable extent — for
  example `OVL_001C_0B52` is 127/126 bytes in the ledger but 118 bytes / 41
  instructions through its RETF; the queue reports the ledger values and
  `instructions` as bounded-decode metadata, except for the explicitly
  hash-checked executable spans in `function_ledger.EXECUTABLE_SPANS`.
  A first RETF alone is not a safe general truncation rule.


## Six-span ledger follow-up

The [reviewed corrections](QUEUE_METADATA_REPAIR.md#six-reviewed-executable-extents)
now cover U1C +271F, U6A +0E37, U38 +1730/+1363/+2EDA and U7D +05F0.
Hash-pinned executable spans are applied before feature decoding. Four are
strict and excluded; the visible rows are U7D +05F0 at 232 bytes / 102
instructions and U38 +2EDA at 113 / 47. These corrections confer no readiness
or credit. The regenerated schema-9 snapshot retains 459 candidates,
57 composable rows, nine interface-caveat rows and fourteen explicit
indirect-blocker rows. Ranked tables and git pins above are historical.


## U38 +25BC extent follow-up

The [U38 +25BC correction](QUEUE_METADATA_REPAIR.md#u38-25bc-follow-up)
changes its ledger extent from 84 bytes / 34 instructions to 73 / 29 using
the accepted body hash before feature decoding. Its eleven-byte tail is
excluded and separately hashed. On main 83306859 this original is already
strict, so it is excluded from the candidate queue. Regeneration on the
current 235/281 main yields 459 candidates and 57 composable IDs; the 467/65
and 465/63 figures are snapshots that predate the later identity/credit
acceptances through #1136.
This follow-up changes no identity, strict map, readiness or accounting.

## Checker availability before handoff

A proposed row must have a runnable checker on the current main baseline before
review. Fetch main, update the author branch to include it, then run the row gate
using a Python environment with Capstone 5.0.7 and Unicorn 2.1.4:

```sh
git fetch origin main
python3 tools/overlay/check_checker_builds.py OVL_0038_12B2 --output resources/checker-builds.json
```

The gate refuses a HEAD that does not include fetched main, unknown row IDs,
compiler errors, load/bind errors, and checkers that do not confirm completion.
It discovers literal `CHECKER_ROWS = ("OVL_0038_12B2",)` registrations in the
actual Python checker files under `tests/recovered/` and `tools/overlay/`.
Each registered file implements `--compile-only` using the same build function,
compiler source list, dynamic-library load and exported-symbol binding as its
normal audit. It prints `CHECKER_BUILD_OK` only after those operations succeed.
There is no second source list or stand-in compiler command to drift out of sync.

The initial registry covers two rows and five build paths: +12B2's differential
and native checkers, and +2B20's differential, native and raw-closure audit builds.
The raw-closure native helper shares the latter audit's library. Existing
unregistered rows are not claimed covered. Every newly proposed row must register
its actual checkers; passing another row's gate is insufficient. The CI
`checker-builds` job runs all registered builds with no game media, and also
requires registration for any manifest row added or changed relative to fetched
main. It does not run native emulation or differential comparisons.

The JSON receipt pins fetched main, HEAD and checker file hashes. Compile success
alone is insufficient: shared libraries may retain unresolved symbols until load,
so the gate loads and binds the real entry points. Synthetic regressions exercise
this failure, missing compiler inputs, absent completion markers, unknown IDs and
new unregistered manifest rows. A build pass establishes checker availability;
it does not establish parity, coverage, identity or credit. For uncommitted work,
the receipt's checker hashes identify the files actually tested.

The +12B2 completeness pilot measured zero newly ready callers (460 candidates /
58 composable on its tested baseline), so its declaration remains documentation
only; see [its measured coverage and limits](U38_EFFECT_MATCH_12B2.md).
Future full-model declarations must first run the before/after readiness
counterfactual and unlock at least one caller. Record a zero delta and skip the
manifest row. Authors make completeness claims; reviewers accept them. Neither a
manifest declaration nor a checker-build pass adds credit.
