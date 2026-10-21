# Reviewed RES_ names for near-only resident bodies (t30; proposal for par-e, VP i20369)

**Author: par-d. Reviewer and lander: par-e.** This is a naming and classification-path change. It grants no identity and
no credit, so the accounting totals do not move. Basis: main `4db0d9fa` (re-measured on the merge with #1341, 02EB accepted), owner media, a fresh export regenerated on this tree.

## The gap

The credit table (`strict_full_c_addresses.csv`) is keyed by `RES_<segment>_<offset>` aliases, and the resident audit
mints a `RES_` name only for an **overlay-called** target. A resident reached only by near calls is named `RPRO_<image>`
(raw prologue) or `RNEAR_<image>`, so the credit join fails on "absent from queue" however good its evidence is. All 28
resident credits on main are overlay-called (checked against the ledger's `kind`): **no near-only resident has ever been
creditable.** The six t30 side (i) bodies below were the first to ask.

Rejected alternative (VP): crediting by the `RPRO_` seed id. A raw-prologue id is a discovery artifact, not a reviewed
identity; the reviewed `RES_` name is what should carry a credit.

## The change

- `resident_callable_coverage.py`: an **explicit list** `REVIEWED_NEAR_ONLY` of six image offsets, mapped to `RES_0802_0040`,
  `0105`, `0193`, `050F`, `05CD`, `06E7`. The row keeps its place, boundary and counts; it gets the reviewed id and a
  `discovery_id` carrying the old `RPRO_` name, so no alias is lost.
- `reviewed_near_only()` refuses a listed body that has no row in `RES_0802_IDENTITY_ARGUMENTS.md` or whose
  image bytes do not match that row's body hash, and the audit refuses a listed body that is not a raw prologue with no
  far caller and at least one near caller.
- Ownership is **not** set by the name. A renamed row is `game_verified` only if its `(segment, offset)` is in
  `GAME_SPECIFIC_TARGETS`, the same channel as every other resident. None of the six is in it, so all six stay
  `unresolved` until the accepting review adds them one at a time.
- `function_queue.add_residents` and `function_alias_audit.address_registry` accept the optional `discovery_id` as a second
  alias of the same original. `strict_candidate_queue.resident_ledger_ids` now returns only `overlay_called` rows, which is
  what its docstring and the near-called-only refusal always meant; the six renamed rows are not overlay-called entries.
- Regenerated: `resident_callable_ledger.csv` (six ids renamed, nothing else) and two values in
  `function_accounting_checkpoint.json` (`queue_sha256`, and the `resident_audit` artifact hash). The checkpoint's
  counts and `reviewed_change` block are untouched. **par-e: reproduce both with `function_queue_audit.py`, which checks
  the pin; I edited the checkpoint by hand only because the refresh tool requires an identity or credit delta.**

## The six conditions

| # | Condition (VP) | Check that pins it |
| ---: | --- | --- |
| 1 | explicit list, reviewed arguments only, never a pattern over RPRO ids | `test_the_reviewed_list_is_exactly_the_six_and_carries_arguments`; `test_row_count_and_every_other_raw_prologue_are_unchanged` (every other raw prologue keeps its `RPRO_` id and has no `discovery_id`); `test_a_listing_without_an_argument_or_with_a_wrong_body_is_refused` (0020 has no row; a wrong body hash; an overlay-called body) |
| 2 | queue row count unchanged | `test_a_reviewed_name_adds_an_alias_and_no_row` (synthetic: equal rows and equal summary, other rows byte-equal); `test_row_count_and_...` (145 audit rows); measured below |
| 3 | an unreviewed RPRO body stays unresolved | `test_an_unreviewed_body_is_not_creditable_by_default` (a credit by the alias is refused "lacks verified game identity"; a credit for an unnamed body is "absent from queue"); `test_the_alias_is_not_an_identity_grant` |
| 4 | classification consults `GAME_SPECIFIC_TARGETS` | `test_ownership_comes_only_from_game_specific_targets` (granting one target flips exactly that row: `game_verified` 29 to 30, `unresolved` down by one, nothing else) |
| 5 | author and acceptor differ | par-d authors; par-e reviews and lands. No `GAME_SPECIFIC_TARGETS`, identity or credit row is touched here |
| 6 | totals before and after, deltas named | measured below |

Mutation check: classifying a renamed row by its name alone is caught by the generator's partition pin; renaming by a
pattern over `RPRO_080*` ids is caught by the near-only refusal. A shared mechanism, named: tests 3 and 4 share the
queue's own ownership rule with the code they test; the independent catcher is the alias audit's registry, built from the
resident rows rather than the queue, which accepts the new alias (below).

## Measured delta (whole queue)

| | before (main) | after |
| --- | ---: | ---: |
| queue originals (`queue_originals`) | 898 | 898 |
| `summary` block (grades, ownership, classes, credits, identities) | | identical to before |
| rows that differ | | **6**, alias list only (`RES_IMAGE_08060, 08125, 081B3, 0852F, 085ED, 08707`) |
| aliases checked by `function_alias_audit` | 2,200 | 2,206 |
| strict / verified identities | 262 / 307 (after #1341; measured 261 / 306 before it) | unchanged by this change |
| resident ledger | 145 rows | 145 rows, six ids renamed |

No other row's classification, ownership, grade or credit moved. The alias audit (independent registry) passes on the
new export with 898 starts, 0 duplicate originals and 0 duplicate credits.

## Scope: how many near-only residents, how many reviewed

**39** resident originals are near-only (a raw prologue or near start with no overlay or loader far caller and at least one
decoded near caller); 18 more non-`RES_` rows are reached by a loader far call, and 2 have no caller seen. By segment:

| Segment | Near-only | Status today |
| --- | ---: | --- |
| 0802 | 9 | **6 reviewed arguments (this change)**; 3 side (ii) never-credit (0000, 0020, 0175), which take no identity |
| 08AE | 7 | the driver segment: `driver_internal` class, never credits |
| 0A40 | 6 | linked CRT, `linked_crt_inferred` (library, no game identity) |
| 00BA | 9 | **no reviewed argument** |
| 0464 | 5 | **no reviewed argument** |
| 0709 | 3 | **no reviewed argument** |

So the answer is not thirty: **six have a reviewed argument today, and 17 game-segment near-only residents (00BA 9, 0464 5,
0709 3) have none.** Those 17 have the same gap and would need their own arguments and a list entry before they could be
credited; nothing here enrolls them.

## What the accepting review does

For each accepted body, in the landing order of `RES_0802_IDENTITY_ARGUMENTS.md`: add `(0x802, <offset>)` to
`GAME_SPECIFIC_TARGETS`, regenerate (the audit then reports it `game_verified`), add the identity row and the credit row
under the `RES_0802_*` name, and refresh the checkpoint. `RES_0802_02EB`, `025D` and `0716` are overlay-called and already
have `RES_` names; they need no list entry.
