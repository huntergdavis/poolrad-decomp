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
| CRT, inherited inferences, not exact vendor-object proof | 9 | Queue `classification=linked_crt_inferred`; census CRT segment tier |
| Doubtful segment-policy exclusion; ownership unresolved | 1 | `RES_IMAGE_0AA20`, alias `RPRO_0AA20`, loaded image `0x0AA20` in census library segment `0xAA2` |

The overlay-manager row remains `unresolved` / ownership `unknown` in the
queue. Its exclusion is an explicit census-policy join, not a classifier
repair. Library inference is not an exact stock-Borland source match.

**Weakest exclusion — `RES_IMAGE_0AA20`: excluded by segment policy;
ownership unresolved; doubtful.** Its queue grade is U and every ingress
counter is zero. Neither the queue nor the resident ledger independently
establishes a library identity. `kind_evidence` in both public CSVs labels
that limit explicitly. The count is **808 callable queue bodies with this
census policy, or 809 if this one exclusion is removed** (library exclusions
would then be 89). The latter is a sensitivity result, not a revision of the
accepted 815 CEILING or an identity grant.

The ten non-SYSTEM **soft exclusions** are listed individually below. For
the first nine, the alias is the exact row ID in
`resident_callable_ledger.csv`, whose `classification` is
`linked_crt_inferred`; the canonical queue preserves that field. Their CRT
library-tier treatment is read from the census. The tenth instead needs the
census's overlay-manager segment rule; its queue classification is not CRT.

| Queue ID | Resident ledger row / alias | Exact exclusion evidence |
| --- | --- | --- |
| `RES_IMAGE_0A400` | `RPRO_0A400` | `classification=linked_crt_inferred` |
| `RES_IMAGE_0A42E` | `RNEAR_0A42E` | `classification=linked_crt_inferred` |
| `RES_IMAGE_0A497` | `RNEAR_0A497` | `classification=linked_crt_inferred` |
| `RES_IMAGE_0A4E5` | `RNEAR_0A4E5` | `classification=linked_crt_inferred` |
| `RES_IMAGE_0A69C` | `RES_0A40_029C` | `classification=linked_crt_inferred` |
| `RES_IMAGE_0A6BD` | `RNEAR_0A6BD` | `classification=linked_crt_inferred` |
| `RES_IMAGE_0A6FD` | `RES_0A40_02FD` | `classification=linked_crt_inferred` |
| `RES_IMAGE_0A731` | `RNEAR_0A731` | `classification=linked_crt_inferred` |
| `RES_IMAGE_0AA0B` | `RNEAR_0AA0B` | `classification=linked_crt_inferred` |
| `RES_IMAGE_0AA20` | `RPRO_0AA20` | Excluded by segment policy; ownership unresolved; doubtful. `original_address=image:0x0AA20 = 0x0AA2*16+0`; census segment `0xAA2`; queue `classification=unresolved`; no ingress |

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

Their absence from the queue is an **export-input gap, not a different
function definition**. `function_queue.py:overlay_rows()` emits only
`inventory["units"][*]["functions"]`. As documented in census correction 1,
`overlay_inventory.find_units()` scans the packed START.EXE image; unit
`0x16`'s descriptor is visible only after loader expansion. The inventory
therefore omits both of that unit's bodies. For residents,
`function_queue_audit.py` delegates to
`resident_callable_coverage.audit_coverage()`: its initial set is
`resident_targets + unmatched_prologues`, followed by candidate-seeded near
closure. It does not add INT 21h AH=25h vector targets as seeds. The census
does add that ingress: `RES_08AE_137C` installs `13EA`, which reaches the
four near helpers. The queue exporter consumes `rows + runtime_rows` from
the narrower audit; it does not merge the census corrections. This explains
the seven omissions without altering either export or the CEILING.

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
   When no C filename appears in those arguments, the existing
   `checker_execution.source_map` resolves explicit declarations and local
   helper imports, including a production body compiled through stdin.
   A mapper result with any conservative fallback is never used to promote
   a row. Direct compiler filenames and explicit document mappings remain
   usable independently of such a fallback.
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
| Callable | 0 | 489 | 0 | 29 | 290 | 808 |
| Library inferred | 0 | 53 | 0 | 37 | 0 | 90 |
| Data | 0 | 0 | 0 | 0 | 0 | 0 |
| Duplicate / alias | 0 | 0 | 0 | 0 | 0 | 0 |
| Unresolved kind | 0 | 0 | 0 | 0 | 0 | 0 |
| Queue total | 0 | 542 | 0 | 66 | 290 | 898 |

The zero `bytes_recovered` is a consequence of the precedence rule, **not
absence of recorded bytes**. In the public CSV, 181 rows carry both
`recorded_body_bytes` and `recorded_body_sha256`: 169 select `credited`, and
12 select `C_drafted`. None remains exclusively bytes-only. This overlap
count covers the explicit machine-readable manifests; it does not say the
other credited rows lack body hashes in their prose evidence. Five examples
of each stronger state follow; full hashes are in the CSV.

| ID | Selected state | Recorded body bytes | Recorded SHA-256 prefix |
| --- | --- | ---: | --- |
| `OVL_0020_0355` | credited | 372 | `72267bd65af83ce8` |
| `OVL_0023_0239` | credited | 278 | `cfce7e60752c92e3` |
| `OVL_0025_0136` | credited | 128 | `0dd25983c54922e1` |
| `OVL_0025_01B6` | credited | 364 | `b1bf2753d995d35f` |
| `OVL_0025_0482` | credited | 143 | `bcfe56989d4733d2` |
| `RES_IMAGE_08020` | C drafted | 32 | `a88fd0caa6f04a9c` |
| `RES_IMAGE_08040` | C drafted | 32 | `94e1c421c6061c22` |
| `RES_IMAGE_08195` | C drafted | 30 | `8f590b56a4323310` |
| `RES_IMAGE_08AE0` | C drafted | 108 | `a8e33767ba204b74` |
| `RES_IMAGE_08B4C` | C drafted | 112 | `f03f1ed3f490c274` |

| State rule | Example and the field that supports it |
| --- | --- |
| Address only | Census-only `OVL_0016_0042`: `SUPPLEMENTAL` address and census correction 1; no queue decode joined |
| Disassembled | `OVL_0017_0000`: overlay ledger `decoded=515` |
| Bytes recovered predicate | `RES_IMAGE_08020`: resident-runtime manifest `body_bytes=32`, `body_sha256=a88fd0ca…`; final label is C drafted because its registered source is present |
| C drafted | `OVL_007D_0000`: partial-model manifest plus `DOCUMENTED_SOURCES`, the documented `pr_u7d_list_timer_0000_cpu` entry point, and its existing C definition |
| Credited | `OVL_0020_0355`: strict map row and canonical queue `strict_full_c=1` |

Source discovery is deliberately limited to explicit registrations and
documented source citations. Unregistered or inline drafts may exist among
rows labelled `disassembled`; this is not an exhaustive negative search for
C implementations. Registrations whose source resolution remains opaque
also stay in that state unless another explicit source mapping suffices;
their declarations are retained and conservative routes appear in the JSON
report. None of these states measures saved-game reachability.

The independent review found a registered-route false negative on the first
frozen head, `6c512c5a`: `RES_IMAGE_0896A` / `RES_0802_094A` compiles the
production `overlay_u25_store_word_0c77.c` through stdin (`-x c -`). The old
filename-argument scan missed it. The author sweep checked 245 registered
queue rows across 218 distinct checkers. Two precise non-filename routes
were found: `res00ba_rec_models_audit.py` (already-credited resident rows)
and `res0802_selector_094a_cpu_audit.py` (the selector). Only the selector
changes final state: **490/28 becomes 489 disassembled / 29 C drafted**.
No credited count changes. The prior 491/27 also omitted the separately
documented U7D source mapping and is superseded.

That is **one `evidence_state` change**, not one changed CSV row. Relative to
`6c512c5a`, 77 rows change metadata: 76 `model_declarations`, seven `c_sources`,
one `evidence_state`, one `state_evidence`, and one `kind_evidence` (field
counts overlap). The six extra source annotations on already-credited
`RES_IMAGE_02F57`, `02F7B`, `031D7`, `039A3`, `039E6`, and `03A2B` leave
their states unchanged; the selector supplies the seventh source change.
No kind, ownership, identity, or strict-credit field changes.

No false C-draft promotion was found under the declared source-presence rule:
existing mappings resolve existing C source through explicit compiler inputs
or document citations. In particular `OVL_03B0_06FB` has a document-to-source
mapping even though its checker import closure is conservative. This is not
an independent semantic review of those C bodies. Conservative source-map
supersets were audited separately and were not adopted as positive evidence.
The JSON records source-resolution routes and hashes the mapper, CMake, and
the entire local Python-helper read universe used for import resolution.

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
other and the published checkpoint. Nine synthetic tests cover stale input,
alias/address collisions, invalid credit, callback/source overstatement, and
equal-byte bodies at distinct addresses, plus precise stdin source discovery
and refusal to promote an opaque CMake superset. The stdin regression was
shown red on the old exporter (`disassembled != C_drafted`), green with the
fix, red after actually reverting the exporter to `6c512c5a`, and green after
restoring it. No compiler or original media executes in that synthetic test.
An independent reader's recount and
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
