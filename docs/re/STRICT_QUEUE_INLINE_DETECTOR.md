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

## Reproduction

    python3 tools/overlay/function_queue_audit.py --exe resources/DOS/POOLRAD/START.EXE --ovr resources/DOS/POOLRAD/GAME.OVR --out-dir resources/fa
    python3 tools/overlay/strict_candidate_queue.py --inventory resources/fa/inventory.json --callgraph resources/fa/callgraph.json \
        --near-calls resources/fa/near.json --ovr resources/DOS/POOLRAD/GAME.OVR --repo . --json resources/queue.json \
        --discards docs/re/strict_queue_discards.json
    python3 -m unittest discover -s tools/overlay -p 'test_strict_candidate_queue.py'
