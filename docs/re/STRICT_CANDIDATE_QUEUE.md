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
runtime targets are recorded as unmapped: the callgraph resolves their segment
stubs but no file/image offset is claimed.

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

**Scoring.** `score = ownership + grade + callee + callers + size - penalty`:
ownership 40/25/5/0 for game_verified/game_evidenced/helper_candidate/
unresolved; grade 20/10/5/0 for A/B/C/D; callee `min(30, 6·strict + 4·modeled
+ 2·verified)` (12 for a leaf, 4 when only unmapped resident/runtime callees);
callers `min(15, 5·fixup) + min(10, 2·aligned PUSH CS) + 2·(near)`; size
12/8/4/2/0 for ≤48/≤128/≤256/≤512/larger bytes; prior penalty 45 main_model,
40 inline_in_strict, 25 unmerged_branch, 18 inline_in_model, 8 commit_mention,
5 source_mention, +5 per additional strong channel, capped at 60. Ties break
on ID. Every row carries its `score_terms`.

**Determinism and pins.** Sorted inputs everywhere; two identical runs
produce byte-identical JSON. The export records SHA-256 for every input file,
the branch-tip table, and the raw `git log` scan. The first pinned export
(queue SHA-256 `16b4c0abecf590ab6f081d24a4b2fe85abe2596656c337a5a888c69d7269a75a`)
was generated from `START.EXE` `d825df2b…a07bd`, `GAME.OVR`
`872964a3…28614`, inventory `6de7664f…ca7f8`, callgraph `c180ff95…f4dc3`,
near audit `a40ba4f2…cbd2c`, with 1,026 refs / 862 tips / 514 unmerged tips
(`tips_sha256` `e6f99d00…d394d`) and 1,979 commits (`commits_sha256`
`4357009f…5393e`).

## First ranked list (top 25 of 600)

| rank | id | unit:offset | bytes | insns | grade | classification | callees (strict/modeled/verified/untranslated) | fixup callers | aligned PUSH CS | prior | score |
| ---: | --- | --- | ---: | ---: | --- | --- | --- | ---: | ---: | --- | ---: |
| 1 | `OVL_005E_263D` | `0x5e:0x263d` | 115 | 41 | A | game_evidenced | 2/0/0/1 | 2 | 7 | source_mention | 82 |
| 2 | `OVL_0025_0000` | `0x25:0x0000` | 310 | 119 | A | game_verified | 1/0/2/0 | 42 | 13 | inline_in_model | 81 |
| 3 | `OVL_03C6_071A` | `0x3c6:0x071a` | 186 | 79 | A | game_evidenced | 1/1/0/1 | 3 | 6 | source_mention | 81 |
| 4 | `OVL_001C_2CCF` | `0x1c:0x2ccf` | 260 | 88 | B | game_verified | 4/0/2/2 | 0 | 1 | source_mention | 79 |
| 5 | `OVL_001C_1EF8` | `0x1c:0x1ef8` | 1747 | 653 | B | game_verified | 1/5/1/1 | 0 | 1 | source_mention | 77 |
| 6 | `OVL_03C6_0AF8` | `0x3c6:0x0af8` | 239 | 98 | A | game_evidenced | 3/0/0/4 | 9 | 0 | source_mention | 77 |
| 7 | `OVL_009E_1869` | `0x9e:0x1869` | 201 | 75 | A | game_evidenced | 2/1/0/1 | 4 | 0 | source_mention | 75 |
| 8 | `OVL_001C_25CB` | `0x1c:0x25cb` | 159 | 66 | B | game_verified | 1/3/1/0 | 0 | 1 | source_mention | 73 |
| 9 | `OVL_009E_0D63` | `0x9e:0x0d63` | 128 | 53 | A | game_evidenced | 1/0/0/1 | 14 | 1 | source_mention | 73 |
| 10 | `OVL_001C_2C03` | `0x1c:0x2c03` | 204 | 74 | B | game_verified | 1/2/1/0 | 0 | 1 | source_mention | 69 |
| 11 | `OVL_006F_1602` | `0x6f:0x1602` | 1878 | 727 | A | game_evidenced | 0/5/2/4 | 1 | 0 | source_mention | 69 |
| 12 | `OVL_009E_0F06` | `0x9e:0x0f06` | 204 | 74 | A | game_evidenced | 1/1/0/3 | 3 | 0 | source_mention | 69 |
| 13 | `OVL_03B4_016D` | `0x3b4:0x016d` | 307 | 114 | A | game_evidenced | 2/0/0/6 | 10 | 0 | source_mention | 69 |
| 14 | `OVL_03BD_05FD` | `0x3bd:0x05fd` | 2059 | 874 | A | game_evidenced | 1/2/0/3 | 5 | 0 | source_mention | 69 |
| 15 | `OVL_005E_08B2` | `0x5e:0x08b2` | 952 | 344 | A | game_evidenced | 3/0/0/5 | 2 | 0 | source_mention | 68 |
| 16 | `OVL_03BD_10C7` | `0x3bd:0x10c7` | 319 | 124 | A | game_verified | 0/0/0/0 | 2 | 0 | commit_mention | 68 |
| 17 | `OVL_001C_2E7D` | `0x1c:0x2e7d` | 176 | 64 | B | game_verified | 2/0/1/0 | 0 | 1 | source_mention | 67 |
| 18 | `OVL_005E_0D23` | `0x5e:0x0d23` | 303 | 102 | A | game_evidenced | 1/0/0/1 | 3 | 1 | source_mention | 67 |
| 19 | `OVL_005E_37E4` | `0x5e:0x37e4` | 494 | 161 | A | game_evidenced | 1/0/0/1 | 3 | 1 | source_mention | 67 |
| 20 | `OVL_0086_0C20` | `0x86:0x0c20` | 815 | 322 | A | game_evidenced | 2/0/0/6 | 6 | 0 | source_mention | 67 |
| 21 | `OVL_009E_0AE7` | `0x9e:0x0ae7` | 360 | 164 | A | game_evidenced | 1/1/0/3 | 4 | 0 | source_mention | 67 |
| 22 | `OVL_03BD_0E08` | `0x3bd:0x0e08` | 703 | 278 | A | game_verified | 0/0/0/1 | 8 | 0 | commit_mention | 67 |
| 23 | `OVL_03C6_10A7` | `0x3c6:0x10a7` | 497 | 191 | A | game_evidenced | 1/1/0/1 | 3 | 0 | source_mention | 67 |
| 24 | `OVL_001C_2F2D` | `0x1c:0x2f2d` | 580 | 273 | B | game_verified | 1/27/21/0 | 0 | 1 | inline_in_model | 66 |
| 25 | `OVL_006F_069E` | `0x6f:0x069e` | 1102 | 411 | A | game_evidenced | 0/3/0/1 | 2 | 1 | source_mention | 66 |

Queue summary: 600 candidates — classification game_evidenced 342,
game_verified 60, helper_candidate 71, unresolved 127; grades A 348, B 250,
D 2; prior status main_model 84, inline_in_model 57, inline_in_strict 4,
unmerged_branch 39, commit_mention 12, source_mention 404; 842 fixup-backed
and 871 aligned-PUSH-CS caller sites across the queue. Near-call evidence is
`decoded_validated` for this export.

## Claude's traps, as detected

| trap | queue evidence |
| --- | --- |
| U25 `+07BC` inlined in strict `+0E0C` | `inline_in_strict`, covered by `OVL_0025_0E0C`; mentions at `src/recovered/overlay_u25_load_word_0e0c.c:1,405`; its one near caller is the documented `+0E4A` aligned `PUSH CS` call; rank 573, score 5 |
| U33 `+0B97` | `unmerged_branch`: `src/recovered/overlay_u33_pair_writer_0b97.c` and docs on `codex/fullc-u33-0b97` |
| U33 `+0221` | `unmerged_branch`: `overlay_u33_clipped_writer_0221.c` on `codex/fullc-u33-0221` |
| U33 `+1403` | `unmerged_branch`: `overlay_u33_both_outside_1403.c` on `codex/u33-1403-retf-wrap` |
| U6C `+3FB3` | `unmerged_branch` on `codex/u6c-record-tier-3fb3` plus commit `010074dc` ("Compose complete U6C +3FB3 caller…"); callee `+3F31` strict |
| U6C `+408C` | `unmerged_branch` on `codex/u6c-record-weighted-408c` plus commit `ed314c24`; callee `+3F31` strict |
| U38 `+12B2` | `unmerged_branch`: `overlay_u38_effect_match_12b2.c` on `codex/fullc-u38-12b2`; callee `+0C06` strict |
| U38 `+25BC` / `+264B` / `+2A87` / `+2D88` | callee `OVL_0038_101D` readiness **modeled** — the merged provisional target-selector helper that composes the resident selector through its hook; each also has an unmerged attempt on its `codex/fullc-u38-*` branch, reported separately |

The `+101D` helper itself appears as `main_model` (rank 598): its merged
provisional full-C model is why the four callers above read as callee-ready.

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

* Mention scanning is heuristic. Docs mention many addresses, so 404 of 600
  rows carry at least one low-weight `source_mention`; the strong signals are
  own files, strict-model inlines, unmerged branch files and commit matches.
* File-name routine extraction cannot map files that omit the offset (for
  example `overlay_u3bd_wallset.c`); such files contribute no `main_model`
  evidence.
* Commit-message detection requires an explicit ID or an offset with unit
  context; branch file names remain the primary unmerged-translation detector.
* Resident/runtime callees cannot be mapped to image offsets from the
  callgraph, so their readiness stays `unmapped` and scores zero.
* Near call sites remain candidate-based: the decoded audit validates
  instruction boundaries under the prologue-seed assumption, not the seeds
  themselves.
