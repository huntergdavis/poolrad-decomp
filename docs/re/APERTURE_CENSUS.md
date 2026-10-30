# The aperture census: which models can reach A0000..AFFFF through a pointer they were handed (VP i20701, 2026-10-21)

**A green differential on a flat model is NOT evidence that the model is aperture-safe, and never was.** Every flat model's audit runs a
Unicorn oracle that maps A0000..AFFFF as ordinary RAM, so the flat model and the oracle agree about a pointer into the aperture **by
construction** (not by fixture luck). 144/144 equal covers every value except the one that matters here. The census therefore emits
the absence of evidence as a column, beside coverage: `aperture_observable = no` for every flat model, in
[`aperture_observability.csv`](aperture_observability.csv), and a test (`test_aperture_census.py`) fails if a model source is
added without a row. Same discipline as naming an incomplete audit as zero coverage.

## The census (tools/overlay/aperture_census.py)

| | models |
| --- | ---: |
| recovered model sources | 548 |
| **bus-aware** (take `pr_bus`; an aperture pointer reads what the machine reads, observable against an EGA oracle) | 32 |
| **flat** (aperture **not observable in their audits**, marked in the CSV) | 516 |
| flat models whose header or opening comment states an aperture exclusion | 4 |

The subject is not the 516 in isolation but the **pair**, because 0411's shape is a bus-aware body handing a pointer to a FLAT callee:
`aperture_pairs.csv` lists every (bus-aware caller, flat callee) pair: **71 pairs**, of which **46 take a data pointer** (StrLoad, StrStore,
StrCopy, StrCat, Str, Val, Move, FillChar, set membership/expansion, GetMem/FreeMem, node and operand readers). For each of the 46 the
pointer's **source** is classified, because only a caller **argument**, a **global** (DS:5D92, the script pointer) or a pointer the callee
**returns** can be corrupt; a body-built **local** or a **code literal** (a string in the caller's own segment) cannot reach the aperture
except through stack aliasing, which the headers exclude.

| Pointer source | pairs |
| --- | ---: |
| can reach the window under a corrupt pointer (caller argument / global / callee-returned) | **27** |
| of those, the caller's header **declares** the exclusion (a precondition) | 25 |
| of those, **not declared anywhere** | **2** |
| unreachable by construction (code literal or body-built local) | 19 |

**The two undeclared, reachable pairs (the finding):** `overlay_u7f_number_0294` (0294, the numeric line editor) and
`resident_0709_printer_0768` (the text-window printer) each hand **StrStore a caller-supplied far string** (`[BP+8]` and `[SP+4]`) with no
statement that it must lie outside A0000..AFFFF. A pointer into the aperture would be read flat by StrStore while the machine reads the
planes. Both are bus-aware bodies with the EGA oracle, so the case is observable: an aperture string pointer that must DISAGREE with the oracle
(a negative control) and a header precondition are the fix; neither model is changed here (a fix ships with its case).
The other 25 reachable pairs carry the declaration ("windows exclude A0000..AFFFF because the callee reads flat RAM"), which is a
**precondition, not a proof**: 0464's blit alone turns it into a stop (`CONTRACT` when a Move meets A0000).

## What this does not yet measure

- **The classification is by reading, partly by header.** 19 of the 46 pairs were classified from the caller's call sites (`classified_by = read`),
  27 from the header's argument description (`header`); a pair misclassified as unreachable would be a defect hidden by the table, so the
  `header` ones are the next to read.
- **"Can reach" is static.** How many of the 27 actually diverge from the EGA oracle under a corrupt pointer is a fixture result, not a
  count of sites: the adversarial fixture is a caller-supplied pointer placed (a) in the aperture and (b) inside a block the body returns,
  frees or rewrites. **0239** carries both kinds (alias cases inside the GetMem block, and `hole-item-in-aperture`/`hole-character-in-aperture`
  negative controls that must disagree with the oracle); its report states the measured result. The other pairs' fixtures are pending.
- **A flat audit cannot reproduce a machine read of the aperture.** For a flat callee the only real evidence is a bus-aware CALLER audit with a
  pointer in the aperture compared against the EGA oracle, and that exists only for the 32 callers that own the oracle.

## Reading the files

`aperture_observability.csv`: `model, kind, aperture_observable, declared_exclusion`. `aperture_pairs.csv`: `caller, flat_callee,
callee_takes_pointer, pointer_source, classified_by, reach, caller_declares_exclusion`. Regenerate with `aperture_census.py --write`.

## Who writes the rows (t582, 2026-10-29)

The CSVs are **derived from the sources**, so a body PR no longer carries them. Before t582 every body PR regenerated
`aperture_observability.csv` itself; six open PRs each held a row that sorted next to the others', so every merge staled the
remaining heads and, with head-bound reviews, the rebase-and-re-verify cost grew quadratically.

- **A body PR** adds its model and leaves the CSV alone. PR CI runs the census test with `APERTURE_CENSUS_ALLOW_MISSING=1`,
  which tolerates **absent** rows only: a committed row with a *wrong* value still fails, on a PR as on `main`. The same
  tolerance is available by hand as `aperture_census.py --allow-missing`.
- **`main`** regenerates the rows: [`.github/workflows/aperture-census.yml`](../../.github/workflows/aperture-census.yml) runs
  `aperture_census.py --write` on every push to `main` and, if the CSVs changed, files or updates one `census/refresh` PR and
  arms auto-merge.

What that job does **not** do: it cannot write to `main`. `main` requires two approving reviews and the `dunkingsheep/review`
status with `enforce_admins` on, so "the census is regenerated within one run of a merge" means **the refresh PR exists within
one run**; it lands when two reviewer lanes accept it like any other PR. A PR opened with `GITHUB_TOKEN` does not trigger
workflows, so the review task for it is filed by the dunking sheep review timer, which polls open pull requests.

The tolerance is keyed on the **ref, not the event** (t628): `repository-hygiene.yml` triggers on `push:` for every branch as
well as on `pull_request:`, and GitHub runs both on a PR head, so an event-keyed tolerance left the push-event run executing
the strict census and failing on every body PR. Every ref except `refs/heads/main` is tolerant; `main` is always strict.

Two limits the tolerance does **not** cover, stated because a first version of this doc did not: it tolerates an absent
**row**, not an absent **classification**. A new model that adds a pointer pair still fails in both modes with
`unclassified pointer pair … add it to SOURCES`, because that guard protects a different table; and a committed row with a
wrong value is stale in both modes. So a body PR can land without a census row, and cannot land with a wrong one.

The guard this keeps and the guard it moves: a model can no longer be *permanently* missing from the census (the refresh job
writes the row), but it can be missing for the interval between its merge and the refresh PR landing. Reviewing a model PR no
longer needs the CSV; reviewing the refresh PR is about the CSVs alone (see `.github/aperture-census-pr.md`). A refresh that
*changes* an existing value instead of adding a row is not a refresh: it names a census input that moved, and it belongs to the
PR that moved it.
