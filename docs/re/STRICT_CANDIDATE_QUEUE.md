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
the 73 overlay addresses credited in the strict manifest; the nine resident
credits are counted and ignored (this tool is overlay-only).

**Callee readiness.** From callgraph edges whose `src_func` is the candidate:
decoded-validated near calls and fixup-confirmed far calls. Each overlay
target is `strict` (strict manifest), `modeled` (a merged per-routine file in
`src/recovered`, `include/poolrad/recovered` or `tests/recovered`),
`verified` (classification `game_verified`) or `untranslated`. Resident and
runtime targets are recorded as unmapped (the callgraph resolves their segment
stubs but no file/image offset is claimed) and **count as unready**: readiness
reports `ready`, `unready` and `total` over every target, and a body is
`composable` only when no target is untranslated, resident or runtime. This
schema-2 correction (2026-10-17) stops the queue proposing bodies whose
resident/runtime far calls are not yet composable.

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
`round(min(30, 6·strict + 4·modeled + 2·verified) · ready/total)` (12 for a
leaf), so unready targets dilute the callee points; callers
`min(15, 5·fixup) + min(10, 2·aligned PUSH CS) + 2·(near)`; size 12/8/4/2/0
for ≤48/≤128/≤256/≤512/larger bytes; `composable` +6 when every callee is
ready; prior penalty 45 main_model, 40 inline_in_strict, 25 unmerged_branch,
18 inline_in_model, 8 commit_mention, 5 source_mention, +5 per additional
strong channel, capped at 60. Ties break on ID. Every row carries its
`score_terms`.

**Determinism and pins.** Sorted inputs everywhere; two identical runs
produce byte-identical JSON. The export records SHA-256 for every input file,
the branch-tip table, and the raw `git log` scan. The schema-2 export (queue
SHA-256 `6eaa652d5192a6b5394342bb91f8941f02dc8cf66e7e6610de3b9c70daa625f1`)
was generated from `START.EXE` `d825df2b…a07bd`, `GAME.OVR`
`872964a3…28614`, inventory `6de7664f…ca7f8`, callgraph `c180ff95…f4dc3`,
near audit `a40ba4f2…cbd2c`, with 1,031 refs / 866 tips / 518 unmerged tips
(`tips_sha256` `3a2574f9…452999`) and 1,959 commits (`commits_sha256`
`89f7a145…75031`).

## First ranked list (top 25 of 600)

| rank | id | unit:offset | bytes | insns | grade | classification | callees (strict/modeled/verified/untranslated) | ready/unready | composable | fixup callers | aligned PUSH CS | prior | score |
| ---: | --- | --- | ---: | ---: | --- | --- | --- | --- | --- | ---: | ---: | --- | ---: |
| 1 | `OVL_0025_0000` | `0x25:0x0000` | 310 | 119 | A | game_verified | 1/0/2/0 | 3/0 | yes | 42 | 13 | inline_in_model | 87 |
| 2 | `OVL_005E_263D` | `0x5e:0x263d` | 115 | 41 | A | game_evidenced | 2/0/0/1 | 2/1 | no | 2 | 7 | source_mention | 78 |
| 3 | `OVL_03C6_071A` | `0x3c6:0x071a` | 186 | 79 | A | game_evidenced | 1/1/0/1 | 2/1 | no | 3 | 6 | source_mention | 78 |
| 4 | `OVL_001C_2C03` | `0x1c:0x2c03` | 204 | 74 | B | game_verified | 1/2/1/0 | 4/0 | yes | 0 | 1 | source_mention | 75 |
| 5 | `OVL_001C_2F2D` | `0x1c:0x2f2d` | 580 | 273 | B | game_verified | 1/27/21/0 | 49/0 | yes | 0 | 1 | inline_in_model | 72 |
| 6 | `OVL_001C_0292` | `0x1c:0x0292` | 78 | 30 | B | game_verified | 1/3/1/0 | 5/0 | yes | 0 | 1 | inline_in_model | 70 |
| 7 | `OVL_009E_0D63` | `0x9e:0x0d63` | 128 | 53 | A | game_evidenced | 1/0/0/1 | 1/1 | no | 14 | 1 | source_mention | 70 |
| 8 | `OVL_001C_2CCF` | `0x1c:0x2ccf` | 260 | 88 | B | game_verified | 4/0/2/2 | 6/4 | no | 0 | 1 | source_mention | 68 |
| 9 | `OVL_001C_25CB` | `0x1c:0x25cb` | 159 | 66 | B | game_verified | 1/3/1/0 | 5/2 | no | 0 | 1 | source_mention | 67 |
| 10 | `OVL_03BD_0E08` | `0x3bd:0x0e08` | 703 | 278 | A | game_verified | 0/0/0/1 | 0/13 | no | 8 | 0 | commit_mention | 67 |
| 11 | `OVL_001C_0EBC` | `0x1c:0x0ebc` | 76 | 29 | B | game_verified | 1/2/1/0 | 4/0 | yes | 0 | 1 | inline_in_model | 66 |
| 12 | `OVL_009E_1869` | `0x9e:0x1869` | 201 | 75 | A | game_evidenced | 2/1/0/1 | 3/4 | no | 4 | 0 | source_mention | 66 |
| 13 | `OVL_03C6_0AF8` | `0x3c6:0x0af8` | 239 | 98 | A | game_evidenced | 3/0/0/4 | 3/5 | no | 9 | 0 | source_mention | 66 |
| 14 | `OVL_001C_1EF8` | `0x1c:0x1ef8` | 1747 | 653 | B | game_verified | 1/5/1/1 | 7/5 | no | 0 | 1 | source_mention | 65 |
| 15 | `OVL_03BD_10C7` | `0x3bd:0x10c7` | 319 | 124 | A | game_verified | 0/0/0/0 | 0/11 | no | 2 | 0 | commit_mention | 64 |
| 16 | `OVL_005E_0D23` | `0x5e:0x0d23` | 303 | 102 | A | game_evidenced | 1/0/0/1 | 1/3 | no | 3 | 1 | source_mention | 63 |
| 17 | `OVL_005E_37E4` | `0x5e:0x37e4` | 494 | 161 | A | game_evidenced | 1/0/0/1 | 1/2 | no | 3 | 1 | source_mention | 63 |
| 18 | `OVL_006C_3291` | `0x6c:0x3291` | 342 | 100 | A | game_evidenced | 0/0/0/2 | 0/2 | no | 7 | 2 | source_mention | 63 |
| 19 | `OVL_0077_004A` | `0x77:0x004a` | 1219 | 495 | A | game_evidenced | 0/0/0/4 | 0/15 | no | 3 | 3 | source_mention | 63 |
| 20 | `OVL_009E_1656` | `0x9e:0x1656` | 263 | 109 | A | game_evidenced | 0/0/0/3 | 0/9 | no | 7 | 2 | source_mention | 63 |
| 21 | `OVL_03B0_10D1` | `0x3b0:0x10d1` | 111 | 52 | A | game_evidenced | 0/0/0/1 | 0/4 | no | 26 | 0 | source_mention | 63 |
| 22 | `OVL_005E_1884` | `0x5e:0x1884` | 666 | 226 | A | game_evidenced | 0/0/0/5 | 0/13 | no | 2 | 6 | source_mention | 62 |
| 23 | `OVL_03BD_05FD` | `0x3bd:0x05fd` | 2059 | 874 | A | game_evidenced | 1/2/0/3 | 3/3 | no | 5 | 0 | source_mention | 62 |
| 24 | `OVL_03C6_10A7` | `0x3c6:0x10a7` | 497 | 191 | A | game_evidenced | 1/1/0/1 | 2/2 | no | 3 | 0 | source_mention | 62 |
| 25 | `OVL_001C_2E7D` | `0x1c:0x2e7d` | 176 | 64 | B | game_verified | 2/0/1/0 | 3/2 | no | 0 | 1 | source_mention | 61 |

Queue summary (schema 2): 600 candidates, 115 composable — classification
game_evidenced 342, game_verified 60, helper_candidate 71, unresolved 127;
grades A 348, B 250, D 2; prior status main_model 84, inline_in_model 56,
inline_in_strict 4, unmerged_branch 46, commit_mention 13, source_mention
397; 842 fixup-backed and 871 aligned-PUSH-CS caller sites across the queue.
Near-call evidence is `decoded_validated` for this export.

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

Readiness correction (schema 2, 2026-10-17): resident and runtime far calls
count as unready — e.g. `OVL_0033_1E31` (ready/unready 3/14) and the U9E dice
candidates `OVL_009E_1344/1656/0AE7` are `composable: false` and no longer
rank as composition-ready; composable bodies carry a +6 score term.

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
* Resident/runtime callees cannot be mapped to image offsets from the
  callgraph; they count as `unready` (diluting callee points and clearing
  `composable`) but carry no offset.
* Near call sites remain candidate-based: the decoded audit validates
  instruction boundaries under the prologue-seed assumption, not the seeds
  themselves.
