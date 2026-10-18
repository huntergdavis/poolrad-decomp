# Strict full-C candidate queue

`tools/overlay/strict_candidate_queue.py` ranks the **non-strict overlay
originals** as strict full-C work candidates and records, for each one, any
prior translation found in the main tree, inlined bodies, unmerged branches or
commit messages. It answers "what should the next strict translation be, and
what has already been attempted?" — not "how complete is the game?". The
whole-game denominator stays open and no completion percentage is calculated.

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
C endpoint is not a raw-CPU callee (for example `OVL_0068_0109` exposes
`overlay_u68_map_update.h` scalar arguments and a segment view without
CPU/stack effects, `OVL_0025_1815`/`+18EB` expose `ecl_helpers.h` semantic
compare APIs with `+1815` also needing the unmodeled runtime string compare
`0AF8:0C10`, and `OVL_03C6_0BE7`/`+0C0F`/`+0C37` expose semantic slot/word
getters), so a caller cannot be composed by handing it the live CPU and RAM
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
count as ready; `interface_caveat`, `resident_interface_caveat`, `partial`,
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
schema-8 export (queue SHA-256
`03a47f97b12f8146c2257382ab98c1356f6157443ce55762726c0f42601c96fc`,
on the 208/258 main after #885–#1041 plus the phantom-edge filter, the
remaining raw-CPU interface caveats and the modeled-system-runtime coverage)
was generated from `START.EXE` `d825df2b…a07bd`, `GAME.OVR` `872964a3…28614`,
inventory `6de7664f…ca7f8`, callgraph `c180ff95…f4dc3`, near audit
`a40ba4f2…cbd2c`, with 1469 refs / 1080 tips / 521 unmerged tips (`tips_sha256`
`0261bc1e…`) and 2400 commits (`commits_sha256` `d74fbc71…`). Git pins are point-in-time;
other lanes keep pushing refs.

## First ranked list (top 25 of 478)

| rank | id | unit:offset | bytes | insns | grade | classification | callees (strict/modeled/verified/untranslated) | ready/unready | composable | fixup callers | aligned PUSH CS | prior | score |
| ---: | --- | --- | ---: | ---: | --- | --- | --- | --- | --- | ---: | ---: | --- | ---: |
| 1 | `OVL_03B4_016D` | `0x3b4:0x016d` | 307 | 114 | A | game_evidenced | 5/0/0/3 | 5/3 | no | 10 | 0 | source_mention | 76 |
| 2 | `OVL_001C_19B0` | `0x1c:0x19b0` | 1025 | 405 | B | game_verified | 4/0/0/0 | 11/4 | no | 0 | 1 | source_mention | 71 |
| 3 | `OVL_0086_059C` | `0x86:0x059c` | 397 | 154 | A | game_evidenced | 3/0/0/0 | 3/0 | yes | 1 | 0 | source_mention | 71 |
| 4 | `OVL_009E_0D63` | `0x9e:0x0d63` | 128 | 53 | A | game_evidenced | 1/0/0/1 | 1/1 | no | 14 | 1 | source_mention | 70 |
| 5 | `OVL_009E_1869` | `0x9e:0x1869` | 201 | 75 | A | game_evidenced | 3/0/0/0 | 4/3 | no | 4 | 0 | source_mention | 70 |
| 6 | `OVL_03BD_0E08` | `0x3bd:0x0e08` | 703 | 278 | A | game_verified | 0/0/0/1 | 6/7 | no | 8 | 0 | commit_mention | 70 |
| 7 | `OVL_001C_25CB` | `0x1c:0x25cb` | 159 | 66 | B | game_verified | 3/0/0/0 | 5/2 | no | 0 | 1 | source_mention | 69 |
| 8 | `OVL_005E_0D23` | `0x5e:0x0d23` | 303 | 102 | A | game_evidenced | 1/0/0/1 | 3/1 | no | 3 | 1 | source_mention | 69 |
| 9 | `OVL_03C6_02D5` | `0x3c6:0x02d5` | 203 | 87 | A | game_evidenced | 2/0/0/2 | 2/2 | no | 6 | 1 | source_mention | 69 |
| 10 | `OVL_001C_2CCF` | `0x1c:0x2ccf` | 260 | 88 | B | game_verified | 5/0/0/2 | 5/4 | no | 0 | 1 | source_mention | 68 |
| 11 | `OVL_001C_2E7D` | `0x1c:0x2e7d` | 176 | 64 | B | game_verified | 3/0/0/0 | 4/1 | no | 0 | 1 | source_mention | 68 |
| 12 | `OVL_009E_1656` | `0x9e:0x1656` | 263 | 109 | A | game_evidenced | 1/0/0/1 | 4/5 | no | 7 | 2 | source_mention | 68 |
| 13 | `OVL_001C_1435` | `0x1c:0x1435` | 500 | 177 | B | game_verified | 3/0/0/1 | 4/1 | no | 0 | 1 | source_mention | 67 |
| 14 | `OVL_001C_1EF8` | `0x1c:0x1ef8` | 1747 | 653 | B | game_verified | 5/0/0/1 | 7/5 | no | 0 | 1 | source_mention | 67 |
| 15 | `OVL_009E_0AE7` | `0x9e:0x0ae7` | 360 | 164 | A | game_evidenced | 2/0/0/3 | 4/3 | no | 4 | 0 | source_mention | 66 |
| 16 | `OVL_009E_0F06` | `0x9e:0x0f06` | 204 | 74 | A | game_evidenced | 2/0/0/2 | 4/5 | no | 3 | 0 | source_mention | 66 |
| 17 | `OVL_03BD_10C7` | `0x3bd:0x10c7` | 319 | 124 | A | game_verified | 0/0/0/0 | 5/6 | no | 2 | 0 | commit_mention | 66 |
| 18 | `OVL_005E_08B2` | `0x5e:0x08b2` | 952 | 344 | A | game_evidenced | 4/0/0/2 | 6/5 | no | 2 | 0 | source_mention | 65 |
| 19 | `OVL_009E_115C` | `0x9e:0x115c` | 488 | 189 | A | game_evidenced | 2/0/0/0 | 2/1 | no | 4 | 0 | source_mention | 65 |
| 20 | `OVL_03B0_10D1` | `0x3b0:0x10d1` | 111 | 52 | A | game_evidenced | 0/0/0/1 | 3/1 | no | 26 | 0 | source_mention | 65 |
| 21 | `OVL_001C_0292` | `0x1c:0x0292` | 78 | 30 | B | game_verified | 4/0/0/0 | 4/1 | no | 0 | 1 | inline_in_model | 64 |
| 22 | `OVL_001C_2C03` | `0x1c:0x2c03` | 204 | 74 | B | game_verified | 3/0/0/0 | 3/1 | no | 0 | 1 | commit_mention | 64 |
| 23 | `OVL_0077_004A` | `0x77:0x004a` | 1219 | 495 | A | game_evidenced | 0/0/0/4 | 4/11 | no | 3 | 3 | source_mention | 64 |
| 24 | `OVL_005E_37E4` | `0x5e:0x37e4` | 494 | 161 | A | game_evidenced | 1/0/0/1 | 1/2 | no | 3 | 1 | source_mention | 63 |
| 25 | `OVL_006C_3291` | `0x6c:0x3291` | 342 | 100 | A | game_evidenced | 0/0/0/2 | 0/2 | no | 7 | 2 | source_mention | 63 |

Queue summary (schema 8, on the 208/258 main after #885–#1041 and the
phantom-edge filter plus the remaining raw-CPU interface caveats and modeled
system-runtime coverage): 478 candidates, 69 would be composable without the
caveats and 67 are composable — 9 bodies carry an interface caveat;
classification game_evidenced 317, game_verified 49, helper_candidate 46,
unresolved 66; grades A 291, B 185, D 2; prior status main_model 74,
inline_in_model 34, inline_in_strict 33, unmerged_branch 11, commit_mention 18,
source_mention 308; 610 fixup-backed and 636 aligned-PUSH-CS caller sites across
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
| `OVL_0086_08C3` | 28 | 26 | 462 / 177 | 16402 / 7096 | 14 | 7 |
| `OVL_03B0_03BD` | 8 | 8 | 830 / 298 | 1393 / 505 | 2 | 2 |
| `OVL_009E_0000` | 8 | 5 | 38 / 16 | 38 / 16 | 1 | 1 |
| `OVL_009E_02FA` | 7 | 6 | 2029 / 911 | 2029 / 911 | 1 | 1 |
| `OVL_009E_1344` | 6 | 5 | 786 / 301 | 7205 / 3023 | 9 | 6 |
| `OVL_03B0_10D1` | 5 | 5 | 111 / 52 | 1504 / 557 | 2 | 3 |
| `OVL_009E_115C` | 4 | 4 | 488 / 189 | 488 / 189 | 1 | 1 |
| `OVL_009E_175D` | 4 | 4 | 268 / 94 | 306 / 110 | 1 | 2 |
| `OVL_0038_0058` | 4 | 4 | 181 / 62 | 6600 / 2784 | 9 | 6 |
| `OVL_0038_1545` | 4 | 0 | 166 / 67 | 8923 / 3815 | 10 | 7 |
| `OVL_0025_054E` | 3 | 0 | 353 / 124 | 5899 / 2393 | 8 | 5 |
| `OVL_005E_1884` | 3 | 2 | 666 / 226 | 20549 / 8640 | 24 | 8 |

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
reasons. On the 208/258 main (195 overlay credits) the schema-8 candidate set is 478 and 67 bodies are composable.

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
  `instructions` as bounded-decode metadata, not the reviewed callable body.
