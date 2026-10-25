# Historical folded-flag ledger: proof coverage (t44)

All **109 historical rows in 54 files** in
[FLAG_FOLDED_SITES.tsv](FLAG_FOLDED_SITES.tsv) have an existing proof reference.
[FLAG_FOLDED_COVERAGE.tsv](FLAG_FOLDED_COVERAGE.tsv) makes the per-row mapping
explicit. It reconciles prior work; it does not introduce new recovery credit
or replace the individual proof's conditions.

Original-byte reproduction base: `d67c4647bb66fcf4fd08de930bc0cae6fff602e7`.
The original-byte receipt records the exact tested PR head on this base.
The proof computations are rerun on this tree; earlier-base receipts are
historical and are not substituted for this run.
Historical ledger SHA-256:
`077b9c2f98f1a4b5e2b8fe66de251a88f75e779bba7464fa999576ef4d301843`.
The source line is a historical identifier from that ledger, not the current
line number in a model. One ledger row can describe several original writers.

| Existing proof | Ledger rows | Executable evidence |
| --- | ---: | --- |
| [U3C6 +10A7](U3C6_10A7_FOLDED_FLAGS.md) | 14 | `u3c6_10a7_folded_flags.prove`, including host stops +117F/+11DF |
| [U31 +0255/+0675](FLAG_FOLDED_U31_0255_0675.md) | 12 | shared walker `--preset u31` |
| [U77 +28BE](FLAG_FOLDED_U77_28BE.md) | 6 | `--preset u77` |
| [U3B0 +00CB](FLAG_FOLDED_U3B0_00CB.md) | 5 | `--preset u3b0` |
| [Slice 5](FLAG_FOLDED_SLICE5.md) | 19 | `--preset slice5` |
| [Slice 6](FLAG_FOLDED_SLICE6.md) | 49 | `--preset slice6` |
| [U7F +00A8](FLAG_FOLDED_U7F_00A8.md) | 4 | the five explicit sites in that document |
| Total | 109 | no missing or multiply assigned ledger rows |

## What the index checks

```sh
python3 tools/overlay/folded_flags_coverage.py --check
python3 tools/overlay/folded_flags_coverage.py > docs/re/FLAG_FOLDED_COVERAGE.tsv
```

The generator derives mappings from the existing shared-walker preset labels
and the U3C6 certificate groups; U7F's four documented historical row numbers
are explicit because that proof uses a CLI site list rather than a preset.
Annotations following a row label are retained in the proof, but do not change
its historical file/line identity. Auxiliary writer and callee-entry checks
that are not ledger rows do not add to 109. Duplicate ledger rows, missing
coverage, multiple proof-suite owners and missing documents are refused.

A matching index proves reference coverage only. It does not inspect original
bytes or certify a model's C expressions. Original-byte results come from the
linked proofs and their executable checks. Three media-free negative controls
confirm refusal of an uncovered claim, conflicting proof owners and a missing
evidence document (`test_folded_flags_coverage.py`). BRANCH and EXPOSED results require
the document's explanation: an existing branch/carry expression reproduces the
consumer, an already-computed writer exposes flags, or an explicitly composed
callee overwrites them before an allowed observation. These results must not
be relabelled DEAD to obtain a clean total.

## Reproduction and limits

The existing computations were rerun against the original media, with both
media SHA-256 values matching those in [the U3C6 provenance](U3C6_10A7_FOLDED_FLAGS.md#original-evidence-and-scope).
The U3C6 certificate also rechecked both owned-span hashes and all writer/observer
assertions. Local disassembly and detailed results remain under ignored
`resources/t44-coverage/final-proof-results.json` and `final-proof.log`.

| Existing computation | Checks | DEAD | BRANCH | EXPOSED |
| --- | ---: | ---: | ---: | ---: |
| `u31` | 33 | 26 | 3 | 4 |
| `u3b0` | 26 | 19 | 3 | 4 |
| `slice5` | 71 | 68 | 1 | 2 |
| `slice6` | 211 | 186 | 3 | 22 |
| `u77` | 13 | 8 | 1 | 4 |
| U7F explicit sites | 5 | 3 | 1 | 1 |

U3C6's separate certificate reports 57 writers: 42 dead before observation,
13 branch conditions reproduced, and two already-computed exposed shifts.
Together these are **416 computations**, including auxiliary writers and
callee-entry checks; they are neither 416 distinct instructions nor 416 ledger
rows. The six walker rows above retain 37 EXPOSED results. The cited documents
explain their existing model computations or composition boundaries; the rerun
does not turn those results into a blanket deadness claim.

Reproduce the five presets with `tools/overlay/flag_fold_liveness.py --preset
NAME --json`, using `u31`, `u3b0`, `slice5`, `slice6` and `u77`. The U7F document
contains its exact five-site CLI, and the U3C6 document contains its certificate
and observer checks. Run each heavy command through the shared
`/home/hunter/workspace/poolrad-heavy.sh`. The index check requires no media;
original-byte reproduction requires the owner's files under ignored `resources/`.


The original ledger was found by comment text. It is not an inventory of every
uncommented fold, new source added later, every debugger stop, or asynchronous
interrupt observation. Each linked proof's code-mutability, host-stop, callee
and pinned DOSBox-X flag rules remain its contract. This reconciliation does
not rerun native execution, re-review every model's semantic expressions, or
make a new strict/identity/replacement claim.
