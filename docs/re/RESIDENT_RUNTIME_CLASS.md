# Resident-runtime bodies: a never-credit readiness class for near-called residents (t30)

**What it is.** `resident_runtime` is the never-credit readiness class for NEAR-called resident bodies. It was ruled by
the VP in i20291 and i20300 (t30) and is the sibling of `driver_internal`
([DRIVER_INTERNAL_CLASS.md](DRIVER_INTERNAL_CLASS.md)). A member's subject is a runtime structure (the heap), or a service
that is only ever called with constant arguments, so **no game state enters it** and it can never carry a game identity.
This is the line that refused `06FB` and `RES_0464_2A0D`. A member:
- **composes like a ready dependency**;
- is **never** a credit, so never counted in strict or identity totals.

**The class lands EMPTY.** Its first intended members are t30's side (ii) bodies: `RES_0802_0000` (FreeMem wrapper),
`RES_0802_0020` (GetMem wrapper) and `RES_0802_0175`. 0175's two call sites, both in `0193`, push only the constants
28h/0/18h/0. Each joins with its own fully evidenced body and an acceptance that moves the pinned set.

**Why this is not `driver_internal`.** `driver_internal` is about **authorship**: bodies of the documented 08AE
timer/audio driver ([RES_08AE_IDENTITY_POLICY.md](RES_08AE_IDENTITY_POLICY.md)). `resident_runtime` is about **subject**:
game-segment residents that are runtime in nature, meaning zero-call or heap wrappers and DOS/BIOS-style services, or that
are called only with constant arguments. Their readiness should compose without conferring a game identity. This is the
same distinction that the t30 side (ii) split made for `0000`, `0020` and `0175`.

**Can the two classes ever disagree about the same body?** Not by construction. `DRIVER_INTERNAL_ID` admits only
`RES_08AE_*`, and `RESIDENT_RUNTIME_ID` excludes `RES_08AE_*`, so no id can be a member of both, and each loader refuses the
other's segment. The closure check lets either class's members be ready callees of the other's, and no other relation
exists between them. If a body ever appears to belong in both, for example a runtime-shaped helper inside 08AE, that is a
finding to raise, not a case to resolve by widening either pattern.

## Membership: narrow, enforced in code (`strict_candidate_queue.load_resident_runtime`)

The rules are `driver_internal`'s, shared through one loader (`_load_noncredit_class`):
- ids only from `docs/re/resident_runtime_models.csv` (same columns);
- disjoint from every credit table, declared boundaries and `GAME_SPECIFIC_TARGETS`;
- body/approval provenance;
- an evidence document naming the member and its body hash;
- with the image, a **complete closure**: a member's own callees must be runtime-covered, declared, strict, or a member of
  either never-credit class.

Two rules are specific to this class:
- **Outside 08AE.** An `RES_08AE_*` id is refused; those bodies are `driver_internal`.
- **Near-called only.** An id with a row in `resident_callable_ledger.csv` is an overlay-called entry, which must earn
  identity instead. It is refused, and the test proves it on the real ledger with `RES_0802_0716`.

## Where the class is honoured, and the pricer defect it exposed

A readiness class that lives in the registry but not in the pricer half-exists (VP i20302). This change wires the class
into every reader in one PR:
- **Queue export** (`callee_evidence`, `resident_target_ready`, the transitive pass): a direct target reads
  `resident_runtime` and counts as ready.
- **Credit refusal** (`load_queue`): a member written into `strict_full_c_addresses.csv`,
  `reviewed_function_identities.csv` or `partial_callee_models.csv` is refused, which is proved by attempt.
- **One-hop pricer** (`price_candidates.py`): `READINESS_STATES` and `USABLE_DEPENDENCIES` gain `driver_internal` and
  `resident_runtime`. Its vocabulary drift guard already failed on any fresh export, because the queue has emitted
  `driver_internal` for the 11 direct rows since that class landed. The guard only passed against a stale export. An
  unknown state defaulted to usable, so the one-hop prices were already right.
- **Closure pricer** (`price_row_closures.py`): members of BOTH never-credit classes are ready terminals, listed and never
  walked or priced. It catches them two ways:
  - as a direct readiness: `expand()`'s ready set gains `driver_internal` and `resident_runtime`;
  - wherever the resident walk reaches them, including through a non-member's near calls (`resident()`).

**A deferred member is still a body in the graph.** `RES_08AE_01A5` was deferred from `driver_internal` membership on
purpose. Its near calls still reach members (`137C`, `13B1`), so the pricing walk has to know which of a NON-member's
callees are ready. Deferring a body from a class is a statement about that body. It says nothing about the readiness of
what the body calls. Before this change the walk had no way to tell, and it priced those members through `01A5`.

**The defect, measured** on main `a6b28722` with a fresh export. Before this change the pricer did not know
`driver_internal` existed. It walked members as unready residents and priced them:
- directly, from the 11 rows with a direct `driver_internal` callee;
- through the deferred non-member `RES_08AE_01A5`, whose near calls reach `137C`/`13B1`.

**119 of 440 rows change price; none changes readiness.**

| Delta | Rows |
| ---: | --- |
| -727 B (all eight walked members) | 118 rows |
| -110 B (`137C` + `13B1` through `01A5`) | `OVL_0036_002D` |

Examples: `OVL_0068_073B` 2,181 B to **1,454 B**; `OVL_0086_0C20` 15,180 B to 14,453 B; `OVL_0036_002D` 8,815 B to
8,705 B.

Totals over all 440 rows: 4,563,047 B to 4,477,151 B. **Ranking:**
- 158 row pairs change order;
- 141 rows change rank (the largest gain is `OVL_0068_073B`, 38 places);
- the cheapest end of the ranking (5-byte rows) is unchanged.

The rows directly calling a member: `OVL_001C_2CCF`, `OVL_001C_32F8`, `OVL_005E_0712`, `OVL_005E_1406`, `OVL_005E_26B0`, `OVL_0068_073B`, `OVL_0068_0989`, `OVL_0068_0D6D`, `OVL_0086_0C20`, `OVL_0086_1658`, `OVL_03C6_0E11`.

The -727 B rows: `OVL_001C_179B`, `OVL_001C_2CCF`, `OVL_001C_2DD3`, `OVL_001C_2E10`, `OVL_001C_2F2D`, `OVL_001C_3171`, `OVL_001C_31B4`, `OVL_001C_32F8`, `OVL_001E_0D09`, `OVL_0020_0ECD`, `OVL_0020_1527`, `OVL_0023_053E`, `OVL_002F_007A`, `OVL_002F_01F6`, `OVL_002F_031A`, `OVL_002F_09DF`, `OVL_002F_0D45`, `OVL_0031_000F`, `OVL_0031_0203`, `OVL_0031_03E4`, `OVL_0031_0545`, `OVL_0031_07F2`, `OVL_0031_0B3C`, `OVL_0038_0058`, `OVL_0038_05EB`, `OVL_0038_0775`, `OVL_0038_0B1B`, `OVL_0038_1168`, `OVL_0038_1399`, `OVL_0038_1545`, `OVL_0038_165D`, `OVL_0038_1671`, `OVL_0038_1685`, `OVL_0038_170B`, `OVL_0038_176F`, `OVL_0038_1A19`, `OVL_0038_1A4C`, `OVL_0038_1CAD`, `OVL_0038_2122`, `OVL_0038_23A4`, `OVL_0038_23C2`, `OVL_0038_2610`, `OVL_0038_2C1C`, `OVL_0038_30A5`, `OVL_005E_02F9`, `OVL_005E_0627`, `OVL_005E_0712`, `OVL_005E_08B2`, `OVL_005E_0E82`, `OVL_005E_116A`, `OVL_005E_1406`, `OVL_005E_1884`, `OVL_005E_1E16`, `OVL_005E_20C3`, `OVL_005E_2414`, `OVL_005E_26B0`, `OVL_005E_2C3D`, `OVL_005E_2DFA`, `OVL_005E_3558`, `OVL_005E_39D2`, `OVL_005E_3A86`, `OVL_0068_073B`, `OVL_0068_0989`, `OVL_0068_0D6D`, `OVL_006A_04A1`, `OVL_006A_13F1`, `OVL_006A_1E6F`, `OVL_006C_0161`, `OVL_0077_0AB3`, `OVL_0077_0EF8`, `OVL_0077_1A8A`, `OVL_0086_08C3`, `OVL_0086_0C20`, `OVL_0086_0F4F`, `OVL_0086_100F`, `OVL_0086_1040`, `OVL_0086_10A7`, `OVL_0086_10E9`, `OVL_0086_1123`, `OVL_0086_1162`, `OVL_0086_1190`, `OVL_0086_11DD`, `OVL_0086_13D8`, `OVL_0086_1439`, `OVL_0086_1499`, `OVL_0086_14C7`, `OVL_0086_151B`, `OVL_0086_1658`, `OVL_0086_17E9`, `OVL_0086_1822`, `OVL_0086_185B`, `OVL_0086_190F`, `OVL_0086_19C3`, `OVL_0086_1A16`, `OVL_0086_1A4F`, `OVL_0086_1A8A`, `OVL_0086_1ACC`, `OVL_0086_225D`, `OVL_0086_234C`, `OVL_0086_24D6`, `OVL_0086_2604`, `OVL_0086_263D`, `OVL_0086_266B`, `OVL_0086_2764`, `OVL_0086_2892`, `OVL_0086_28BC`, `OVL_0086_295B`, `OVL_0086_2BB7`, `OVL_0086_2C09`, `OVL_0086_2E02`, `OVL_0086_2F4D`, `OVL_0086_2F99`, `OVL_0086_302A`, `OVL_0086_3058`, `OVL_0086_30A2`, `OVL_0086_30D9`, `OVL_009E_1344`, `OVL_03C6_0E11`.

**t30 is unchanged in substance.** All 21 endpoint-free rows still contain the whole `RES_0802` cluster (1,813 B), and
their three cheapest are still `OVL_0025_0341` 2,043 B, `OVL_006F_0EC4` 2,813 B and `OVL_0025_0411` 2,926 B. Three of the
21 drop 727 B each: `OVL_0077_0AB3`, `OVL_002F_01F6` and `OVL_002F_031A`.

## Proof that the empty class changes nothing else

- On main `a6b28722`, the export written by main's own `strict_candidate_queue.py` and the export written by this
  change are equal in summary, with **0 readiness differences across all 440 rows**.
- The export records one new input hash, `resident_runtime`, so an export written before this change reads as stale and
  must be regenerated.
- With t29's rows landed (#1319), **364 of 440 rows have no uncovered SYSTEM endpoint**: +21 against 343 on the pre-t29
  base `1abb88a0`, which is the figure of record measured in memory before landing.

## Tests

- `test_strict_candidate_queue.ResidentRuntimeTest`:
  - the pinned committed membership (`frozenset()`);
  - narrow and fail-closed membership: 08AE, a non-resident id, the doc, provenance, duplicates, columns, declared,
    game-specific, and an overlay-called entry on the real ledger;
  - closure completeness, refused and accepted, including a GetMem wrapper on covered `0AF8:023F` and a callee in the
    other class;
  - credit refusal in every table;
  - the readiness label and count;
  - no member in a committed credit table.
- `test_price_row_closures.NeverCreditClassesTest`:
  - a near-called member is a terminal, never walked or priced (with a control);
  - a row whose only unready callee is a member prices as ready;
  - a direct `driver_internal` callee is not priced;
  - the committed table is loaded and pinned;
  - on the owner media and a fresh export, `OVL_0068_073B` loses exactly 727 B and `OVL_0036_002D` exactly 110 B.
    The deltas are pinned; the absolute prices are recorded above with their base.
- `test_price_candidates.ReadinessVocabularyTest.test_never_credit_classes_are_known_usable_states`.
- Full `tools/overlay` suite: 206 tests, OK.
- **Mutation-checked:** removing `driver_internal` from the ready set, disabling the member terminal, or disabling the
  near-only refusal each fails a test.

## 0175 against the real 0193 row (t30 side ii, par-d)

Measured on main `f9b145ef` with the owner media; pinned by `test_price_row_closures.RealDisplayRowTest` and
`test_strict_candidate_queue.ResidentRuntimeTest.test_0175_is_refused_on_the_real_image_until_0040_is_ready`.
- **Pricer, no change needed.** Walking the real `0802:0193` with `RES_0802_0175` as a member removes exactly 0175's 30 B:
  bodies `{0193: 90, 0175: 30, 0105: 112, 0040: 197}` become `{0193: 90, 0105: 112, 0040: 197}` and 0175 is listed under
  `runtime_residents`. 0040 stays priced because 0105 also reaches it. Strict (`0899:0059`), unmapped, bad and declared are
  unchanged.
- **Membership cannot land yet.** The real-image closure check refuses 0175: `+018C call 0x40: RES_0802_0040 is not a
  member or strict`. 0175 calls 0040, a side (i) body that is evidence-only. The refusal is pinned; the test must be
  updated when 0040 is ready.
- **Proposed row (TEXT only, review fields blank):** `RES_0802_0175,docs/re/RES_0802_0175_CPU.md,<review_pr>,<reviewed_commit>,<body sha in the doc>,30`.
  The committed table is untouched, so the empty-class pin stands.


## t76 admission: FreeMem and GetMem wrappers

The non-author Codex acceptance adds `RES_0802_0000` and `RES_0802_0020`
from merged PRs #1340 (head `9387ffa8cd9ba532d01b977801bfaa05c768b03a`)
and #1338 (head `a15535841459c1e20b03ecaf487c773f1072b638`). The committed
set becomes exactly **0000, 0020, 0175**; both queue/pricer membership pins
move with the table. Independent reviewer: pi-review-2. This is readiness
membership, never identity or strict credit for these wrappers.

Both are outside 08AE and absent from overlay-called resident ledger rows.
Their complete 32-byte bodies each have 12 starts, no conditional branches
and one runtime-covered heap callee: FreeMem `0AF8:0254` or GetMem `0AF8:023F`.
The real loader-image closure guard accepts the complete three-member set.
A test now runs that guard on the committed membership when media is present;
existing refusal tests continue to reject unresolved callees and credit overlap.

Independent reruns through `poolrad-heavy.sh` reproduce:

| Member | Unicorn | Native | Independent checks | Body starts |
| --- | ---: | ---: | ---: | ---: |
| `RES_0802_0000` | 37 (33 RETURN, 4 RUNERROR) | 18 (15 RETURN, 3 RUNERROR) | 21 | 12/12 |
| `RES_0802_0020` | 48 (41 RETURN, 6 RUNERROR, 1 HEAP_ERROR) | 26 (24 RETURN, 2 RUNERROR) | 32 | 12/12 |

Unicorn compares CPU14 and all 1 MiB with zero flag masks. Native compares
CPU14 plus the documented stack, pointer and heap-variable windows. The
native executable is the declared **unpinned** DOSBox-X 2026.01.02 build,
SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`.
It is not claimed as the pinned source build. Native result hashes exactly
match the body evidence notes. The declared RunError capture, stack bounds,
step caps, and GetMem HeapError stop remain in force.

Evidence and reproducible commands:
[FreeMem wrapper](RES_0802_0000_CPU.md),
[GetMem wrapper](RES_0802_0020_CPU.md). Reports remain under ignored
`resources/t76-runtime/`; no original bytes or native captures are committed.


The isolated runtime-membership comparison on base
`a611ec51b86dfdce65033dc553b2a42ad0f8f22a` uses identical inventory,
callgraph and validated-near inputs with `--no-git`. Before and after:
**431 candidates, 49 composable, 289 strict, 125 replaced**. No direct
callee-readiness row changes. A diagnostic walk starting inside `0716`
(with only that strict root removed from the terminal set so the walk is
non-vacuous) prices `0716:564 + 0020:32 + 0000:32` before and only
`0716:564` after: **628 -> 564 bytes**, exactly the two heap wrappers.
This is a pricing diagnostic, not a new credit or whole-closure completion.
No strict grant is included in this admission.

Final validation on the admission-only tree: the full overlay unittest suite
passes **329 tests, two skips**; the focused real-image membership test also
passes. Canonical `function_queue_audit.py` regenerates all ledgers and the
queue byte-identically, retaining the base accounting pin `4a6f855a…`.
Strict, identity, classifier, checkpoint and REPLACED files are byte-identical
to the base. The ready-helper membership set is untouched and remains empty.
