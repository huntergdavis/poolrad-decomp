A function for this count follows the static census rule: **“distinct callable original function bodies in non-Borland-library code.”** This is the rule behind the **815 CEILING** in [FUNCTION_CEILING_CENSUS.md](FUNCTION_CEILING_CENSUS.md), not a new game-ownership test or a verified denominator.

This reconciliation joins the 898 candidate starts to that reviewed census
policy and current evidence records. `callable` contributes one distinct
non-library body; `data`, `duplicate_alias`, and `library_inferred` contribute
zero; `unresolved` would remain undecided. Equal bytes at different original
addresses remain distinct. The exporter rejects duplicate original addresses
or overlapping aliases rather than silently deduplicating corrupted inputs.
These are census-derived kinds, not 898 independently reviewed classifications.

| Queue result | Count |
| --- | ---: |
| Total candidates | 898 |
| Callable under the census rule | 808 |
| Data | 0 |
| Duplicate / alias originals | 0 |
| Library inferred | 90 |
| Unresolved kind | 0 |
| Callable already strict credited | 290 |
| Callable with reviewed game identity | 335 |
| Callable with unknown game ownership | 473 |

The zeros describe the canonical, already deduplicated queue, not every raw
prologue or entry-table word. Public-entry aliases and resident stub aliases
remain in each row's `aliases` field; they are not additional candidates.
`game_ownership` and `queue_classification` preserve the queue's labels.
No accounting or credit file changes, identity grants, or completion percentage
are part of this reconciliation.

## The excluded set and the seven additions

[function_backlog_excluded.csv](function_backlog_excluded.csv) names all 90
excluded rows, with aliases, `library_tier`, `kind_authority`, and
`kind_evidence`:

| Exclusion | Rows | Exclusion evidence |
| --- | ---: | --- |
| SYSTEM | 80 | Queue `classification=linked_runtime_evidenced`; census SYSTEM segment tier |
| CRT | 9 | Queue `classification=linked_crt_inferred`; census CRT segment tier |
| Overlay manager | 1 | `RES_IMAGE_0AA20`, alias `RPRO_0AA20`, loaded image `0x0AA20` in census library segment `0xAA2` |

The overlay-manager row remains `unresolved` / ownership `unknown` in the
queue. Its exclusion is an explicit census-policy join, not a classifier
repair. Library inference is not an exact stock-Borland source match.

The net difference 83 is **90 exclusions minus seven additions**:
`898 - 90 = 808` queued non-library bodies; `808 + 7 = 815 CEILING`.
[function_backlog_census_only.csv](function_backlog_census_only.csv) lists the
seven addresses read from the census corrections:

| Census-only ID | Recorded ingress |
| --- | --- |
| `OVL_0016_0042` | Heap-error callback via DS:4414 |
| `OVL_0016_00AC` | Unit initialization through resident stub |
| `RES_08AE_13EA` | INT 08h vector installed by `RES_08AE_137C` |
| `RES_08AE_1477` | Timer-handler near helper |
| `RES_08AE_14EC` | Timer-handler near helper |
| `RES_08AE_157A` | Timer-handler near helper |
| `RES_08AE_160B` | Timer-handler near helper; its table targets are interior labels |

These seven rows are separate from the 898-row queue. Their addresses and
ingress are read from the prior census, not re-decoded here; their evidence
state in this join is conservatively `address_only`. The static CEILING
is inherited, and its census limitations continue to apply.

## Evidence states and their source fields

[function_backlog_reconciliation.csv](function_backlog_reconciliation.csv)
contains every queue row. `state_evidence`, `model_declarations`, `c_sources`,
`evidence_docs`, and the recorded span/hash fields expose the basis of each
state. The strongest mapped state wins in this order:

1. `credited`: `strict_full_c_addresses.csv` through the canonical queue's
   `strict_full_c` and evidence-document fields. Verified identity alone does
   not imply this state.
2. `C_drafted`: an explicit non-callback model manifest or checker
   `CHECKER_ROWS` registration, with existing C source resolved from the
   evidence document or the registered checker's parsed compiler arguments.
   SYSTEM/CRT endpoint declarations in `RUNTIME_COVERED_ENDPOINTS` also map
   their named existing source. One explicit `DOCUMENTED_SOURCES` mapping
   resolves `OVL_007D_0000`: its document names
   `pr_u7d_list_timer_0000_cpu`, and the exporter requires that C definition
   in `overlay_u7d_list_timer_0000_cpu.c`. This state includes partial models, bounded
   models, evidence-only models, and registered checkers; it asserts source
   presence, not correctness, full behavior, readiness, or credit. A build
   list can include dependencies; it is not a per-source ownership claim.
3. `bytes_recovered`: a recorded exact body length and SHA-256 in a reviewed
   identity/model manifest, when no stronger state maps. Those hashes are read
   as prior evidence, not rehashed against media here.
4. `disassembled`: positive overlay ledger `decoded`, or the resident audit's
   candidate-seeded / SYSTEM-closure decoded path. This does not certify an
   exact body extent or game ownership.
5. `address_only`: no stronger evidence maps. A callback declaration alone,
   a source mention alone, and a span bound alone do not promote the row.

| Kind | Address only | Disassembled | Bytes recovered | C drafted | Credited | Total |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Callable | 0 | 490 | 0 | 28 | 290 | 808 |
| Library inferred | 0 | 53 | 0 | 37 | 0 | 90 |
| Data | 0 | 0 | 0 | 0 | 0 | 0 |
| Duplicate / alias | 0 | 0 | 0 | 0 | 0 | 0 |
| Unresolved kind | 0 | 0 | 0 | 0 | 0 | 0 |
| Queue total | 0 | 543 | 0 | 65 | 290 | 898 |

Source discovery is deliberately limited to explicit registrations and
documented source citations. Unregistered or inline drafts may exist among
rows labelled `disassembled`; this is not an exhaustive negative search for
C implementations. None of these states measures saved-game reachability.

## Reproduce and detect stale input

The metadata baseline is main
`52aac3da2c6db39979b072ae49cb0efc905dbef3`. On 2026-10-26 the read-only
primary checkout product at
`../poolrad-decomp/resources/function-accounting-current/queue.json` was stale:

| Queue | SHA-256 | Strict / verified identities |
| --- | --- | ---: |
| Observed stale private product | `e85812a57fda5068237cac456ec9646433a07fd662516bbe8333295e9b2ec4d1` | 244 / 290 |
| Regenerated canonical product | `ce7a5462da1f8f8b85c187c1d476a3b8fc8c4ddd0a6877ab5b5c638520ca3049` | 290 / 335 |

The published checkpoint and credit map carry 290 strict credits; the stale
private queue does not. The primary checkout was not modified. The exporter
reads only the explicitly supplied queue, rejects the stale hash, verifies
all product hashes against the checkpoint, and matches current ledger,
classifier, strict-map, and reviewed-identity file hashes. The stale primary
product is a historical observation, not a required reproduction input.

With owner-supplied hash-matched media under ignored `resources/DOS/POOLRAD/`
and Capstone 5.0.7, regenerate the canonical inputs in a clean checkout:

```sh
../poolrad-heavy.sh python3 tools/overlay/function_queue_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --out-dir resources/function-accounting-current
python3 tools/overlay/function_backlog_reconciliation.py \
  --queue resources/function-accounting-current/queue.json --check
python3 -m unittest tools/overlay/test_function_backlog_reconciliation.py
```

Omit `--check` only when intentionally regenerating the four public metadata
products. The tool prints the complete kind-by-state table as JSON.
[function_backlog_reconciliation.json](function_backlog_reconciliation.json)
records both media hashes, every canonical queue-product hash, the exporter
and source-mapping tool hashes, every evidence/source file read by the join,
and the three CSV output hashes. Neither input bytes nor decoded instructions
are copied into these products.

The canonical audit re-derived the inventory, call graph, validated near
calls, overlay ledger/classifier, and resident callable audit; the three
ledgers matched the tree byte-for-byte and two queue exports matched each
other and the published checkpoint. Seven synthetic tests cover stale input,
alias/address collisions, invalid credit, callback/source overstatement, and
equal-byte bodies at distinct addresses. An independent reader's recount and
exact-head media receipt belong in the PR verdict, not in this author's count.

**Not verified:** fresh classification of every callable start; exact stock
library provenance; new game ownership; exact extents or hashes beyond the
upstream audit; exhaustive unregistered/inline C drafts; native/CPU parity;
natural gameplay reachability; the seven census-only bodies' bytes; or an
independent-reader acceptance. The 815 CEILING is not a verified denominator.

Reused prior-session context: Dunking Sheep query
`poolrad function backlog reconciliation 898 originals function_queue FUNCTION_CEILING_CENSUS candidate starts evidence state`
returned Codex session `01a1281b-85e3-7e23-bdf6-1ea8af4d69de` (PR #377's
898-start queue and open denominator) and the original census session
`a3ad1536-abb2-4fb8-af2c-1362b8656678`. Current counts were re-derived as
above; census policy, ingress descriptions, and alias rules were read from
[the ceiling census](FUNCTION_CEILING_CENSUS.md),
[queue contract](FUNCTION_CENSUS_QUEUE.md), and
[alias audit](FUNCTION_ALIAS_AUDIT.md).
