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

## Extension to fourteen: the eight nested bodies of credited `RES_00BA_0BB8` (VP i20397)

par-a ruled these eight PASS in [NEAR_ONLY_GAME_SEGMENT_IDENTITY.md](NEAR_ONLY_GAME_SEGMENT_IDENTITY.md): factor 1 is direct
(near sites inside the strict-credited `0BB8` and `13AB`), factor 2 is the 285-byte character object and the `+C8h` list
records. The mechanism and the six conditions are unchanged; the list grows from six to fourteen entries, each explicit.
The verdict page records each body's size and image offset but not its hash, so the hash is recorded here and
**cross-checked against the 115C checker's own pins** (`PINS` in `u9e_character_pair_115c_audit.py`, an independent source),
which a test compares.

| id | verdict | body_sha256 | body_bytes |
| --- | --- | --- | ---: |
| RES_00BA_0000 | docs/re/NEAR_ONLY_GAME_SEGMENT_IDENTITY.md | dc1d899f356871351977cc5ffdba6f1445bdc9e51a0f42f0758deaae63f28917 | 507 |
| RES_00BA_01FB | docs/re/NEAR_ONLY_GAME_SEGMENT_IDENTITY.md | 231ccedb42ed924477cf8c0db9d2415eb541fc21e58a52af19052ba2dad9a21f | 135 |
| RES_00BA_0282 | docs/re/NEAR_ONLY_GAME_SEGMENT_IDENTITY.md | 92bfa47643e8418793ce20a0ad79475c84ef340046e61327682efb132f01cf3a | 286 |
| RES_00BA_03A0 | docs/re/NEAR_ONLY_GAME_SEGMENT_IDENTITY.md | 56c28fb6c276cd2242687ca09e1cb39adf8b0f22d19e9a3dddd1feecb9ac1f98 | 141 |
| RES_00BA_10E6 | docs/re/NEAR_ONLY_GAME_SEGMENT_IDENTITY.md | fac332383b03ed081b3c863de7a7f086277f2a3db34505e6a0ecb6959c9f101c | 130 |
| RES_00BA_11EA | docs/re/NEAR_ONLY_GAME_SEGMENT_IDENTITY.md | 8331051d67bce5f352938484858963a8e7e6758d43e47bd131f72b9c2980e020 | 152 |
| RES_00BA_1282 | docs/re/NEAR_ONLY_GAME_SEGMENT_IDENTITY.md | 3c9c2c535ca66703d223b5e65c9fa6b13e04d8cc05603f8a88c6024376059bf4 | 163 |
| RES_00BA_1325 | docs/re/NEAR_ONLY_GAME_SEGMENT_IDENTITY.md | bf71db2bfc2033faa964f74e185be79e0cd2789ff3a6c0c5d5ff828b19b53a36 | 134 |

1,648 B in total. Ownership is still not set by the name: none of the eight is in `GAME_SPECIFIC_TARGETS`, so all stay
`unresolved` until par-e grants them one at a time. The loader refuses an 00BA entry that has no PASS verdict naming its
image offset (the `RPRO_` id) and size in par-a's page. The other nine of the 17 stay BLOCKED and are not listed.

**Measured delta of the extension** (main `6c0b335e`, fresh export before and after): 898 originals before and after;
`summary` block identical; exactly **8** rows differ (`RES_IMAGE_00BA0, 00D9B, 00E22, 00F40, 01C86, 01D8A, 01E22, 01EC5`),
alias list only; aliases 2,206 to 2,214; strict 262 and identities 307 unchanged. The independent alias audit passes
(898 starts, 0 duplicate originals, 0 duplicate credits). Updated scope: of the 17 game-segment near-only residents,
**8 now carry a reviewed name and 9 remain BLOCKED** (par-a); 14 of the 39 near-only residents are listed in all.

## Extension to sixteen: two nested bodies of the printer `0709:0768` (VP i20405)

par-a's pass 2 and pass 3 ([NEAR_ONLY_GAME_SEGMENT_IDENTITY.md](NEAR_ONLY_GAME_SEGMENT_IDENTITY.md)) ruled `RES_0709_06A7` PASS
(draws the next character of 0768's message through glyph `01DF` at the window cursor `DS:5E24/5E25`) and `RES_0709_0714`
PASS **with the 01DA limit**: it skips blanks in 0768's local copy of its string argument, and only the text that
`game_verified` `001C_0A09` passes (far `DS:70C8`, ECL string buffer 1) is claimed; `001C_1EF8`'s stack buffers and CS
constant are not. The limit travels with the name: listing it grants no claim beyond what par-a's page states.

**Hash source, disclosed:** the 115C pins cover only `00BA`, so these two have **no independent pin**. Each hash is
computed from the loader-expanded image over par-a's recorded size; the span decodes cleanly (41 and 19 instructions, `RETF 2`
at the end of each, a Pascal prologue at the start), and the 26 bytes between `0714` and `0768` are a string literal, not code.

| id | verdict | body_sha256 | body_bytes |
| --- | --- | --- | ---: |
| RES_0709_06A7 | docs/re/NEAR_ONLY_GAME_SEGMENT_IDENTITY.md | c9674dcc914fe3e3bb0ed3d54e58776e351f7d3888ccfd15d5b4320d97241bc5 | 109 |
| RES_0709_0714 | docs/re/NEAR_ONLY_GAME_SEGMENT_IDENTITY.md | 67b1ce78e79b1e281051f3bc73e634213d904cedd5ec265a4bdf72f502af799c | 58 |

Ownership is still not set by the name; neither is in `GAME_SPECIFIC_TARGETS`. Tally over the 17 game-segment residents:
10 PASS (the 0BB8 eight, `06A7`, `0714`), 5 FAIL, 2 BLOCKED.

**Measured delta of this extension** (main `d0432662`, fresh export before and after): 898 originals before and after; `summary` identical;
exactly **2** rows differ (`RES_IMAGE_07737`, `RES_IMAGE_077A4`), alias list only; aliases 2,214 to 2,216; strict 262 and
identities 307 unchanged; the independent alias audit passes. The list is now sixteen entries.

## The second door: loader-called residents (VP i20420)

**Two doors, one mechanism, in one sentence:** a resident that no overlay calls has no `RES_` name, so it is creditable only
through the explicit reviewed lists in `resident_callable_coverage.py`, by the **near-only door** (reached by near calls
alone, `REVIEWED_NEAR_ONLY`) or the **loader-called door** (reached by far calls relocated inside the resident image, with
no overlay caller, `REVIEWED_LOADER_CALLED`); a third kind, if one turns up, is a new entry condition on these lists, not a
third hand-made mechanism. par-e counted 18 loader-called rows on main (`RPRO_03C3E`, `03DA0`, `03ED0`, `04277`, `04354`,
`0470F`, `0708A`, `07101`, `07130`, `07147`, `08012`, `08232`, `08988`, `08ACC`, `08C5B`, `08CB2`, `0A310`, `0A400`).
**Only one has a reviewed argument and a PASS verdict today**, so the list has one entry; the other seventeen stay under their
`RPRO_` ids, unresolved and unlisted, and nothing here enrolls them.

| id | evidence | body_sha256 | body_bytes |
| --- | --- | --- | ---: |
| RES_0709_00B7 | docs/re/RES_0709_RECT_FILL_00B7_CPU.md | f26747c1e8836399f118763dfee5a2bdeace551495f75b5533772b888fa65ff5 | 186 |

`RES_0709_00B7` (`RPRO_07147`, image `[7147,7201)`) is the rectangle fill on the game's draw page; par-a's pass 4 rules it
PASS: both loader-relocated far calls (`03F1:01F1`, `03F1:020C`) lie inside strict-credited `RES_03F1_0137`, and it reads and
restores the page state `DS:4A1A`/`4A18`. **Hash source:** this row's hash is the one pinned by the evidence document's own
body table (the 00B7 original-byte audit, 202 Unicorn cases), which a test compares; it is independent of the loader. Each door
has its own entry condition, checked on the image: the near-only door refuses a body with any relocated far caller, the
loader-called door refuses one with none, and the audit refuses a name in the wrong list. Ownership is still not set by the
name: `RES_0709_00B7` is not in `GAME_SPECIFIC_TARGETS`.

### A defect in the first two extensions' verdict check, found while adding this one

The loader's message said "no PASS verdict", but the check it ran only looked for the body's **inventory row** (the first
table of the verdict page: `RES_xxxx (RPRO_xxxxx) | bytes |`), which lists FAIL and BLOCKED bodies as well. A failed body
could therefore have been listed without the loader noticing. **No listed body was affected**: all 17 listed bodies outside
0802 do have a PASS verdict (a test now proves it), and no FAIL body was ever listed. The check is now `verdict_of`: the
**last** verdict row whose first cell is the body (PASS under a `PASS` heading or bold `PASS`; FAIL; BLOCKED; not assessed),
so a later pass supersedes an earlier BLOCKED and a mention in another row's text is not a verdict. Negative controls on the
real page: `RES_0464_1509`, `1484`, `1596` and `RES_0709_0000` read FAIL and `RES_0464_007B` BLOCKED. This is the
claim "the loader requires a PASS verdict" made true after the fact; until this change it was only true of the data.

**Measured delta of the second door** (main `dc24b0d2`, fresh export before and after): 898 originals before and after;
`summary` identical; exactly **1** row differs (`RES_IMAGE_07147`), alias list only (`RES_0709_00B7` added beside `RPRO_07147`);
aliases 2,216 to 2,217; strict 262 and identities 307 unchanged; the independent alias audit passes (898 starts, 0 duplicate
originals, 0 duplicate credits). Ledger: one id renamed; checkpoint: the same two values re-pinned, `function_queue_audit.py` rc 0.
