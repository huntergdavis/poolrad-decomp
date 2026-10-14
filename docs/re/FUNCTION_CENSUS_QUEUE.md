# Reproducible original-function queue

`tools/overlay/function_queue.py` exports a deterministic JSON queue from the
existing overlay inventory, evidence ledger and classification CSV. It can
also reconcile the resident callable audit and its SYSTEM closure. The queue
is a set of candidate original addresses for work assignment, **not a complete
game-function denominator**. No completion percentage is calculated.

## Stable identity and evidence

An overlay original is `OVL_<four-digit unit>_<four-digit relative offset>`;
for example `OVL_0025_06D3`. Its existing `0x25:0x06d3` ledger ID remains an
alias. Public entry slots and their raw words are recorded separately. An
entry word equal to a candidate offset plus `0x800` resolves to that candidate;
it does not add an original. Runtime overlay stubs are `STUB_<segment>_<offset>`
aliases of their overlay original. The 404 slots therefore remain separate
from the 673 candidate-start count.

A resident original is `RES_IMAGE_<five-digit loaded-image offset>`. Existing
`RES_`, `RPRO_`, `RNEAR_`, `SYS_` and `SYSNEAR_` identifiers are aliases joined
by loaded image address. A resident-looking start on a known overlay entry
stub joins the overlay original. File offsets, loaded image offsets and overlay
relative offsets are different coordinate systems; equal numbers alone do not
merge them. These identities are scoped to both executable input hashes.

Overlay grades preserve the existing ledger rules: A is public-entry or
fixup-confirmed far-call evidence; B is near-call-only; C is procedure-pointer-only;
D is entry-alias-only; U has none of those. Resident grades use A for an
overlay/loader far call or initialization entry, B for decoded near calls, and
U otherwise. `start_evidence` retains the prologue/boundary evidence separately.
Grades rank start evidence, not game ownership or translation completeness.

Near-call evidence defaults to `raw_unvalidated`. Supplying the hash-matched
`validate_near_calls.py` report promotes overlay near calls only when its
failure/difference counts are zero and its decoded/raw site totals match the
ledger. That audit still assumes the candidate prologue seeds are valid starts.
Resident paths are likewise labeled `decoded_candidate_seeded`.

Only `game_verified` yields verified game ownership. Provisional overlay game
evidence, helper candidates and unresolved classifications keep
`game_or_library=unknown`. CRT and SYSTEM-closure rows are `library_inferred`;
these labels are not exact stock-library matches. Duplicate resident aliases
retain the maximum per-address evidence count rather than summing repeated
observations. Conflicting game/library classifications fail the export.

## Strict full-C mapping

[strict_full_c_addresses.csv](strict_full_c_addresses.csv) is an explicit,
reviewable mapping from 66 original addresses to their reviewed evidence
documents: 57 overlay and nine resident. It reuses the strict count in
[the battle card](../BATTLE_CARD.md), the numbered full translations in
[FUNCTION_LEDGER.md](FUNCTION_LEDGER.md), the reviewed unit-0x38 batches,
and the subsequent address-specific notes linked in each manifest row.
The four unit-0x3C6 credits are the accessors documented in
[U9E_0160.md](U9E_0160.md), not their bounded caller.

Verified identity does not automatically receive strict credit. Bounded
models, extracted cores, provisional string translations, unresolved callee
effects and source-ownership candidates are excluded. In particular the ECL
runner, dispatcher, store hook, operand loader, unit-0x3BD entries 6/7 and
resident `RES_0464` models gain no strict credit here. A split or combined C
implementation still receives credit once per reviewed original address.

Every mapped ID must resolve to a verified game original. Missing mappings,
duplicate credits through aliases, duplicate input IDs, inconsistent grades,
address-set mismatches, negative evidence counts and unsupported build hashes
fail before output is written. Overlay-only exports apply the 57 overlay
credits; combined exports also validate and apply the nine resident credits.
Each export records SHA-256 digests of its exact metadata inputs. CSV digests
provide reproducibility, not independent proof that CSV evidence is correct;
the owner-media audit below regenerates that evidence.

## Reproduce

Synthetic checks require only Python:

```sh
python3 -m unittest tools/overlay/test_function_queue.py tools/overlay/test_classify_overlay.py
```

With Capstone 5.0.7 available, regenerate from ignored owner-supplied DOS files:

```sh
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/function_queue_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --out-dir resources/function-census-audit
```

The audit checks START.EXE SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`
and GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`
before generating products. It regenerates the inventory, callgraph, decoded
near-call report, overlay ledger/classifier and resident callable audit;
compares all three CSVs byte-for-byte with their checked-in counterparts;
and requires two queue exports from identical inputs to be byte-identical.
The existing resident audit already checks its 145 non-SYSTEM candidates
and 80 SYSTEM-closure seeds; this change exposes the latter as address rows.
Input media and decoded instructions are never exported. Local JSON/CSV/log
products stay under ignored `resources/`; stdout contains hashes/counts only.

The local audit measured this disjoint address partition on 2026-10-14:

| Queue region | Original starts | Verified game | Library inferred | Unknown ownership | Strict full-C |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overlay candidates | 673 | 118 | 0 | 555 | 57 |
| Non-SYSTEM resident candidates | 145 | 10 | 9 | 126 | 9 |
| SYSTEM closure seeds | 80 | 0 | 80 | 0 | 0 |
| Combined queue | 898 | 128 | 89 | 681 | 66 |

Overlay grades remain A=402, B=269, D=2. No real-data address aliases
collide across these three observed regions; synthetic tests exercise the
resident/runtime and overlay-stub deduplication cases. The 898 observed
starts include library code and unresolved candidates and are not a game total.

The combined `queue.json` SHA-256 after two identical exports was
`5df46c8f516f0670f2974eeae9d2cab3504c39a10404f197f63ae3c9cca74a77`.

For an overlay-only queue using those regenerated products:

```sh
python3 tools/overlay/function_queue.py \
  --inventory resources/function-census-audit/inventory.json \
  --ledger docs/re/overlay_function_ledger.csv \
  --classification docs/re/overlay_classification.csv \
  --near-calls resources/function-census-audit/near.json \
  --json resources/function-census-audit/overlay-queue.json
```

Prior-session lookup was attempted with
`deja "poolrad function census 673 404 stable queue denominator resident aliases"`
and MCP recall; neither returned before the bounded lookup was stopped. No
prior-session findings are claimed as reused. The implementation reuses the
checked-in [overlay ledger](OVERLAY_FUNCTION_LEDGER.md),
[classification](OVERLAY_CLASSIFICATION.md),
[resident coverage](RESIDENT_CALLABLE_COVERAGE.md), and the hash-locked loader
audit. Unseeded code, indirect entries and unresolved authorship keep the
whole-game denominator open. This queue adds no game-identity or strict-C
credit to the existing checkpoint.
