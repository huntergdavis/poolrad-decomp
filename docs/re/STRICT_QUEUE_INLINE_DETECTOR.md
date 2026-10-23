# Strict queue: bodies already translated inside credited bodies (detector fix, discard report, measurement)

Assigned by PM i22017/i22023/i22026 (VP i22025). The strict candidate queue's `prior_translation.inline` detector missed
`OVL_0025_07BC`. That body is fully translated as `load_hook()` inside the strict model of `OVL_0025_0E0C`, whose registered
checker pins its span by hash. Any lane could claim it as new work; par-d2 did (i22013) and stopped before writing code.

## The fix

`is_boundary_mention()` discards a source mention whose shape only names an address. The shapes are:
- a call-site map, a span hash, an end assertion or a range comment;
- since 7f1fd9d6, an `ip =` redirect or a bare comment.

Both mentions of `+07BC` in the `+0E0C` model are such shapes: the header comment "load hook +07BC" and `c->ip = 0x07BCu`.

**New rule.** A boundary-shaped mention still counts as inline evidence when the covering routine's **registered checker**
pins a span that starts at the candidate's offset in the same unit, as `(0x07BC, 0x0A74, "<sha256>")`. The registered
checkers come from the CI registry (`check_checker_builds.discover`, both folders). A checker that pins a span by hash
executes those original bytes against its C, so the body is translated there. The `IP_REDIRECT` comment, which used
`0x07BC` as its example of a non-modelling redirect, now states the exception.

**The 13 priors that 7f1fd9d6 removed, by ID.** They were reproduced by running that commit's queue and its parent's on
the same products:

`OVL_001C_0292`, `OVL_001C_0BD1`, `OVL_001C_0EBC`, `OVL_001C_13D7`, `OVL_001C_179B`, `OVL_001C_2B7E`, `OVL_001C_2F2D`,
`OVL_0025_07BC`, `OVL_0025_0A74`, `OVL_005E_0000`, `OVL_007F_0000`, `OVL_007F_0054`, `OVL_009E_0FD2`.

The new rule restores **exactly one** of them, `OVL_0025_07BC` (pinned by `OVL_0025_0E0C`). No same-unit registered checker
pins any of the other twelve, which stay removed. `test_the_13_priors_7f1fd9d6_removed_stay_removed_except_07bc` pins that
exact outcome on the real tree.

`OVL_0025_0A74` (the store hook) is probably translated inside `overlay_u25_store_word_0c77.c`, whose header says so.
`+0C77` is not credited, though, and no registered checker pins `+0A74`, so the rule does not count it. It is listed here
as a known residual, not hidden.

## Follow-up (PM i22091)

- **SEG:OFF mentions.** Sources also write far calls as `SEG:OFF` with no `+` or `0x`; par-b found `00BA:23B7` inside
  `overlay_u31_candidate_check_0255_cpu.c` (PM i22086). The mention scan now reads `pppp:oooo` as a unit:offset
  (`007F:0A88`) or as an overlay stub paragraph:entry (`0041:00CA`, from the rows' `STUB_pppp_oooo` aliases), with kinds
  `seg_off` and `stub_seg_off`. A resident `SEG:OFF` such as `00BA:23B7` names no overlay candidate, because this queue
  ranks overlays only, so for residents the pre-claim grep is the check. `test_seg_off_mentions_are_found` pins both forms
  and the non-matches.
- **CI.** `test_strict_candidate_queue.py` runs media-free (two media cases skip). It now runs in the `checker-builds` job.
- **Pin.** `strict_queue_discards.json` carries `pin` (tree commit and export sha256) and `kind`: a pinned measurement,
  not a live list. A regeneration with `--discards` writes both.

## The discard report

Every source mention the detector rejects is reported with its row, file, line, rule and text. A regeneration writes it
with `strict_candidate_queue.py ... --discards <path>`. The rules are:
- the boundary shapes above;
- `NO_SINGLE_ROUTINE`: the file names no single routine;
- `OWN_ROUTINE`: the mention is in the row's own files;
- `COVERING_ROUTINE_UNMODELLED`: the file names a routine that has no model.

The queue summary carries `discarded_mentions`, `discarded_by_rule` and `pinned_span_rescues`, and each row carries
`prior_translation.discarded_mentions`.

The committed copy is `docs/re/strict_queue_discards.json`, measured at the pin below:

| Rule | Mentions |
| --- | ---: |
| OWN_ROUTINE | 375 |
| NO_SINGLE_ROUTINE | 106 |
| COMMENT_ONLY | 50 |
| IP_REDIRECT | 21 |
| RANGE_COMMENT | 13 |
| BOUNDARY_ASSERTION | 6 |
| SPAN_HASH | 5 |
| CALLSITE_MAP | 1 |
| **total** | **577** |

## Measurement

**Pin:** origin/main `c6be5156` (commit dated 2026-10-23 09:23 PDT, fetched before the run), branch head `12b1285f` (this fix rebased on it). Queue
export SHA-256 `555cf1617f33c1c8b8a5d8e0a9469ce5e6ddab1d74cff8581348081c01b9dc9f`, 439 offered candidates. The fresh
function queue has 898 originals.

**1 of 439 offered candidates was already translated inside a credited body:** `OVL_0025_07BC`, host
`src/recovered/overlay_u25_load_word_0e0c.c` (`OVL_0025_0E0C`, strict), pinned by `tools/overlay/u25_load_word_0e0c_audit.py`.

**Precision of the flag:** this signal fired 30 times on 439 candidates; 29 were inspected and were false (neighbour end markers, en-route hashes, call-site comments, coincident constants); 1 was real.

The detector's own flags are wider, and **wrong**. On this export, 30 rows carry inline evidence that names a strict-credited
routine; 25 of them have `inline_in_strict` as their status. All of these flags predate this change. Each mention was read,
and 29 of the 30 are not translations:

| Shape of the mention | Rows |
| --- | --- |
| a neighbour's end or next-body marker (`BODY_END`, `ends[] = {…}`, `END = …`, entry-index or `unit.index` asserts) | `OVL_03C6_052B`, `OVL_03C6_0E11`, `OVL_005E_1884`, `OVL_007D_03D5`, `OVL_005E_1E16`, `OVL_0025_1DF0`, `OVL_0025_2215`, `OVL_0025_20CC`, `OVL_0038_12FB`, `OVL_0038_2610`, `OVL_005E_26B0`, `OVL_03C6_07D4`, `OVL_0038_1086` |
| a span decoded or hashed en route (a caller scan, a natural-route hash) | `OVL_0038_2122`, `OVL_0038_23A4`, `OVL_001C_2DD3`, `OVL_001C_2E10` |
| a call site or caller named in a comment (the body is called, not inlined) | `OVL_0068_0D6D`, `OVL_0031_07F2`, `OVL_0086_008A` |
| a coincident constant (a segment, a flag mask, 0, 1, a RAM-size macro, a type name) | `OVL_03C6_0AF8`, `OVL_0038_0400`, `OVL_002F_0000`, `OVL_03BD_0000`, `OVL_03C6_0000`, `OVL_001C_0001`, `OVL_007F_0020`, `OVL_03BD_031F` |
| the row's own model is on main (status `main_model`; the mention is a neighbour's end marker) | `OVL_0068_0294` |
| **a translation, pinned by a checker** | **`OVL_0025_07BC`** |

These false flags apply the `inline_in_strict` penalty (40 points) to rows that are not translated, so the queue under-ranks
them. Fixing them is a separate change: the same shapes need the boundary rules, and the coincident constants need
address-shaped matching. This change does not alter them, and the count is stated so that nobody reads the 30 as translations.

## The unknown-ownership headline

The fresh function queue at `c6be5156` has **483 rows of unknown ownership out of 898**. That equals the committed checkpoint
(`function_accounting_checkpoint.json`: `unknown_ownership` 483); the "~494" in the request is not what this tree measures.

**The one measured already-translated row, `OVL_0025_07BC`, is one of those 483.** So 483 overstates the unknown, untranslated
work by **at least 1 row** from this mechanism. It is an upper bound in that sense. The detector's raw 30 flags (28 of them in
the unknown set) must not be subtracted: 29 are false.

There are two more cases this measurement does not count:
- bodies translated inside **uncredited** models, such as `OVL_0025_0A74` in `+0C77`;
- bodies that a credited routine executes without a span pin.

## From 483 unknown rows to 439 candidates

This section reads the same exports, at the same pin: c6be5156; function queue `d52ae347…`, the same as the committed
checkpoint; candidate export `555cf161…`. The candidate queue offers **every overlay original that is not strict-credited**:
673 overlay rows − 234 strict = 439. Unknown ownership is a different cut, over both domains, so the two sets differ in two
ways:

| | Rows | Why |
| --- | ---: | --- |
| unknown ownership | 483 | 395 overlay + 88 resident |
| − unknown but not offered | −88 | **all 88 are resident-image rows.** The strict candidate queue ranks overlay originals only (`load_queue`: candidates are overlay rows minus strict credits). These 88 are tracked in the resident inventories, not here |
| + offered but not unknown | +44 | **all 44 are `game_verified` overlay rows without strict credit.** Their ownership is known, and they are still offered |
| = offered candidates | **439** | 483 − 88 + 44; the 395 unknown overlay rows are all offered (no unknown row is strict) |

Unknown, not offered (88, resident): `RES_IMAGE_00FE2`, `RES_IMAGE_01307`, `RES_IMAGE_0148F`, `RES_IMAGE_01513`, `RES_IMAGE_01583`, `RES_IMAGE_01713`, `RES_IMAGE_01C13`, `RES_IMAGE_01C4D`, `RES_IMAGE_0202B`, `RES_IMAGE_0228C`, `RES_IMAGE_02432`, `RES_IMAGE_0257E`, `RES_IMAGE_025C9`, `RES_IMAGE_02B8B`, `RES_IMAGE_02DC8`, `RES_IMAGE_02EEA`, `RES_IMAGE_02FD3`, `RES_IMAGE_030FB`, `RES_IMAGE_03226`, `RES_IMAGE_032CA`, `RES_IMAGE_03379`, `RES_IMAGE_034AB`, `RES_IMAGE_036FE`, `RES_IMAGE_03728`, `RES_IMAGE_037E8`, `RES_IMAGE_03AF7`, `RES_IMAGE_03C3E`, `RES_IMAGE_03DA0`, `RES_IMAGE_03ED0`, `RES_IMAGE_03F10`, `RES_IMAGE_041F8`, `RES_IMAGE_04277`, `RES_IMAGE_04354`, `RES_IMAGE_04640`, `RES_IMAGE_046BB`, `RES_IMAGE_0470F`, `RES_IMAGE_0473F`, `RES_IMAGE_04751`, `RES_IMAGE_048E5`, `RES_IMAGE_04F14`, `RES_IMAGE_053B1`, `RES_IMAGE_05AC4`, `RES_IMAGE_05B49`, `RES_IMAGE_05BD6`, `RES_IMAGE_05C07`, `RES_IMAGE_05F91`, `RES_IMAGE_06192`, `RES_IMAGE_0649C`, `RES_IMAGE_06ACE`, `RES_IMAGE_06C72`, `RES_IMAGE_0704D`, `RES_IMAGE_0708A`, `RES_IMAGE_07090`, `RES_IMAGE_07101`, `RES_IMAGE_07130`, `RES_IMAGE_07147`, `RES_IMAGE_0720C`, `RES_IMAGE_077F8`, `RES_IMAGE_07A73`, `RES_IMAGE_07BF6`, `RES_IMAGE_07C85`, `RES_IMAGE_07CCF`, `RES_IMAGE_07DF8`, `RES_IMAGE_08012`, `RES_IMAGE_08020`, `RES_IMAGE_08040`, `RES_IMAGE_08195`, `RES_IMAGE_08232`, `RES_IMAGE_08376`, `RES_IMAGE_08445`, `RES_IMAGE_0896A`, `RES_IMAGE_0897D`, `RES_IMAGE_08988`, `RES_IMAGE_08AB2`, `RES_IMAGE_08ACC`, `RES_IMAGE_08AE0`, `RES_IMAGE_08B4C`, `RES_IMAGE_08BBC`, `RES_IMAGE_08C5B`, `RES_IMAGE_08C85`, `RES_IMAGE_08CB2`, `RES_IMAGE_09E5C`, `RES_IMAGE_09E91`, `RES_IMAGE_0A222`, `RES_IMAGE_0A28E`, `RES_IMAGE_0A2FA`, `RES_IMAGE_0A310`, `RES_IMAGE_0AA20`.

Offered, not unknown (44, game_verified and not strict): `OVL_001C_0001`, `OVL_001C_0126`, `OVL_001C_01A3`, `OVL_001C_0234`, `OVL_001C_0292`, `OVL_001C_02E0`, `OVL_001C_03B4`, `OVL_001C_0450`, `OVL_001C_0817`, `OVL_001C_0856`, `OVL_001C_0951`, `OVL_001C_0994`, `OVL_001C_0A09`, `OVL_001C_0BD1`, `OVL_001C_0C74`, `OVL_001C_0DEF`, `OVL_001C_0E5E`, `OVL_001C_0EBC`, `OVL_001C_0F08`, `OVL_001C_10D1`, `OVL_001C_12C6`, `OVL_001C_1435`, `OVL_001C_1629`, `OVL_001C_16C9`, `OVL_001C_179B`, `OVL_001C_19B0`, `OVL_001C_1DB1`, `OVL_001C_1EF8`, `OVL_001C_25CB`, `OVL_001C_2775`, `OVL_001C_2AA2`, `OVL_001C_2AC1`, `OVL_001C_2B33`, `OVL_001C_2B7E`, `OVL_001C_2C03`, `OVL_001C_2CCF`, `OVL_001C_2E10`, `OVL_001C_2F2D`, `OVL_001C_3171`, `OVL_0025_0A74`, `OVL_0025_0C77`, `OVL_0036_002D`, `OVL_03BD_0E08`, `OVL_03BD_10C7`.

## Reproduction

    python3 tools/overlay/function_queue_audit.py --exe resources/DOS/POOLRAD/START.EXE --ovr resources/DOS/POOLRAD/GAME.OVR --out-dir resources/fa
    python3 tools/overlay/strict_candidate_queue.py --inventory resources/fa/inventory.json --callgraph resources/fa/callgraph.json \
        --near-calls resources/fa/near.json --ovr resources/DOS/POOLRAD/GAME.OVR --repo . --json resources/queue.json \
        --discards docs/re/strict_queue_discards.json
    python3 -m unittest discover -s tools/overlay -p 'test_strict_candidate_queue.py'
