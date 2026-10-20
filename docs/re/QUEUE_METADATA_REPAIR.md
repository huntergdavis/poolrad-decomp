# Queue span and indirect-dispatch metadata repair

The initial schema-9 hygiene change corrected four planning traps found after the
U3B0 +1140 raw upgrade. It changed queue metadata and two ledger rows; it
does not change recovered C, classification, strict manifests or credit.
VP instructions i18901/i18906 authorize this scope.

## Original-byte evidence

The original build remains START.EXE SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`
and GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.

| ID | Original executable span | Correct bytes / instructions | Old ledger bound / decoded / instructions |
| --- | --- | --- | --- |
| U86 +3092 | [3092,3097), GAME file [172202,172207) | 5 / 4 | 16 / 12 / 7 |
| U6A +0824 | [0824,08BF), GAME file [98389,98544) | 155 / 57 | 230 / 229 / 96 |
| U9E +0000 | [0000,0026), GAME file [175850,175888) | 38 / 16, unchanged | 38 / 38 / 16 |

U86's RETF at +3096 ends the empty procedure. The following 11 bytes form
one length-prefixed Pascal string and are outside the body. U6A's RETF at
+08BE ends the sort; the following 75 bytes are outside the callable body,
consistent with the already-merged
[`overlay_u6a_sort_0824.h`](../../include/poolrad/recovered/overlay_u6a_sort_0824.h).
Those trailing bytes previously inflated its ES-reference feature from 6 to 16.

Executable SHA-256 values:

- U86 +3092: `fc1e97b8fe4158fdf9a46de4d3a1b4a04602fb787f6793778bc6d2b7a758d780`.
- U6A +0824: `c77f6aa238e6d9f11bf01decb9084b02ac0a6cd4a5742611adae6848b0d927d7`.
- U9E +0000: `1572342d9be429f7a7b7f6b015b2a609726b4d5580e34235fda31eb2a43b9217`.

The U86 excluded tail hashes to
`cf5b765801fccc95183b03448f66f4851870ab75f16b62f7c93e30173bdeab08`;
the U6A excluded tail hashes to
`1fa286ec9866a6933b157f045f0345e99383efd1572b688fe862125ea70996c2`.
Original bytes and strings stay private.

## Behavior after repair

`function_ledger.py` applies the two corrections **before decoding features**.
It requires the reviewed end to fit the candidate bound and the body hash
to match. A different body or overlapping next-prologue bound is rejected.
Other routines retain their existing extent; an early RETF does not generally
prove that every later byte is DATA.

The regenerated ledger differs only in the two rows above: bound, decoded
bytes and instruction count for each, plus U6A's corrected ES-reference count.
Evidence grades, input identities and classifications are unchanged.

U9E +0000 executes an indirect LCALL at +001E through DS:6828+4*type.
Its [merged raw prefix](U9E_LIST_RELEASE_0026_CPU.md) stops before that call,
so an empty direct-call graph must not imply composability.
`UNRESOLVED_INDIRECT_DISPATCH` adds one explicit unready target with the
instruction site, reason and evidence document; its destination address is
null because it is live data. Callers name the unresolved-dispatch reason,
and modeled transitive callers remain blocked. An explicit known boundary
takes precedence over an incorrectly assigned strict/full endpoint label.

The U25 +1815 caveat still identifies its semantic-only API. Its SYSTEM
+0C10 dependency is already covered by the merged
`pr_system_strcmp_0c10`; the outdated claim that this runtime is unmodeled
is removed. This wording repair does not make U25 +1815 raw-CPU callable.

## Validation and limits

Fifteen focused synthetic tests pass, including mismatched span hashes,
bounds that cut through a reviewed body, unchanged unreviewed spans,
self/direct/transitive indirect blockers, a stale full-endpoint label,
and retention of the overlay semantic-API caveat.

Original-media regeneration produces U86 5/5/4 and U6A 155/155/57
(bound/decoded/instructions), and U9E +0000 has ready=0, unready=1,
total=1, composable=false. The queue has 467 candidates and 65 composable
rows, compared with 66 before the dispatch correction. Nine rows retain
interface caveats. Fourteen rows explicitly name the indirect blocker:
the dispatcher and thirteen direct callers. This counts affected metadata,
not fourteen newly discovered functions or newly unavailable implementations.

The explicit dispatch map is deliberately narrow. This repair does not
audit all remaining indirect calls or prove that every queue-unready row is
unavailable. Earlier ranked tables and fanout estimates remain snapshots;
the U9E +0000 38-byte body is not a complete handler-closure cost estimate.

Dunking Sheep recall `Poolrad strict candidate queue span overrides U86 3092
U6A 0824 U9E 0000 indirect dispatch 0C10` returned Codex session
`01a1386e-bea7-7883-8ddd-06fd2be8db9b`, Claude session
`281c8700-5acc-438b-b93d-c061e28437eb` and reviewer session
`01a14c56-d196-777f-aaeb-0eac03c2083d`. Reused evidence is the existing
U6A sort extent, U9E dispatch-boundary document and merged SYSTEM raw API;
the ledger span corrections were rechecked against the original bytes here.

## Six reviewed executable extents

VP instruction i18936 authorizes extending the same ledger-generation guard
after #1124 to six already accepted executable spans. This follow-up changes
only those six ledger rows; acceptance records, recovered C, classification,
strict map and accounting remain unchanged. All body hashes below were
rechecked against the pinned GAME.OVR before regeneration.

| Original | Executable unit span | GAME file span | Old bound / decoded / instructions | Correct bytes / instructions | Tail bytes |
| --- | --- | --- | --- | --- | --- |
| [OVL_001C_271F](U1C_DELAY_271F_CPU.md) | [271F,272D) | [13413,13427) | 86 / 86 / 38 | 14 / 6 | 72 |
| [OVL_006A_0E37](U6A_STRING_NODE_0E37.md) | [0E37,0EBE) | [99944,100079) | 389 / 389 / 159 | 135 / 48 | 254 |
| [OVL_0038_1730](U38_ROLL_APPEND_1730_CPU.md) | [1730,175E) | [66443,66489) | 63 / 62 / 29 | 46 / 22 | 17 |
| [OVL_0038_1363](U38_APPEND_1363_CPU.md) | [1363,1382) | [65470,65501) | 54 / 53 / 26 | 31 / 15 | 23 |
| [OVL_007D_05F0](U7D_ENTRY_05F0_CPU.md) | [05F0,06D8) | [148412,148644) | 305 / 304 / 138 | 232 / 102 | 73 |
| [OVL_0038_2EDA](U38_DISPATCH_BOUNDARY_2EDA.md) | [2EDA,2F4B) | [72501,72614) | 125 / 122 / 51 | 113 / 47 | 12 |

| Original | Executable body SHA-256 | Excluded tail SHA-256 |
| --- | --- | --- |
| OVL_001C_271F | aec9bf6c4361ea61af680d3d4aed03b5bdc0f62e5d8b3221bc197845ca34f8b1 | 5c2c68ee56c5956df192668559937b31e33f4b827f7828ecb78c47cb9e3dd343 |
| OVL_006A_0E37 | 5f63f1823287894ede57fcef122307f3e296293a633e530a1836f2f36d389234 | c15c8d8b687431d77130f54082be24678e8ce0df139d754a0e4e1fd053a4e9b6 |
| OVL_0038_1730 | 402e83b7c5036596f9af8cb3da21ea77519e58cca195e5385fc88d593e0e2f25 | 6095847039b5986e3b9a9fcd4fb4da095cad067d8b0b9de5c2cb1e22530ffc01 |
| OVL_0038_1363 | 4850fb98f51666178d3b6e8fc612f3a2afa80e355a7fa13cafb56073481a58c2 | 07fef1b29ba7cd29896475353e1e6e43c30133201487f8ed76c30d42e884acf8 |
| OVL_007D_05F0 | ac3eb0a915fa30d77b06b262429879f2400d3b8584b6e97ba413dde924709c7d | 150b499ae1f3bbae867a4639265da22500c46aaba6b9e562d6d07d0f8342ec75 |
| OVL_0038_2EDA | e75c9f7ebfcd4235a1a5a11b6d77ae544c0bd64a3c3f8ade8c5fac454757ecd3 | 26e004e80d1b0979de8b61640adf7b98240d73db6c5e47a28caa9bb4d92d05f2 |

The tail ends at the old next-prologue bound. Original bytes and strings stay
private. Each corrected body decodes fully through its documented RETF and
its hash must match before feature extraction. This is an explicit reviewed
exception, not a rule to truncate at the first return. Existing hash/bounds
tests retain that distinction.

Thirty-five related queue tests pass. A new synthetic CLI regression checks
that excluded tail bytes never reach the feature decoder and that the next
routine remains intact. Only bound, decoded bytes, instructions and these
ES-reference counts change: U1C +271F 2 to 0, U38 +1730 1 to 0, and
U6A +0E37 21 to 7. Every other ledger row and column is unchanged; both
earlier #1124 corrections are preserved.

The regenerated schema-9 queue retains 467 candidates, 65 composable rows,
nine interface-caveat rows and fourteen explicit indirect-blocker rows.
Four corrected originals are strict and excluded from this candidate queue.
The visible corrected rows are U7D +05F0 (232 bytes / 102 instructions) and
U38 +2EDA (113 / 47); the latter remains unready. Two frozen no-git exports
are byte-identical. Size changes confer no readiness or credit.

Reused recall: Codex session 01a14af7-41a3-7b92-803d-39037aab3555 located
+1363 author evidence. All six extents/hashes reuse the linked accepted
evidence documents and were rechecked here. Other stale extents disclosed
separately are outside this six-row authorization.


## U38 +25BC follow-up

VP instruction i18951 adds the already reviewed U38 +25BC extent after
#1126. The [raw-selector evidence](U38_TARGET_SELECTOR_CPU.md) pins
[25BC,2605), GAME file [70167,70240), at **73 bytes / 29 instructions**.
RETF 0Ah starts at +2602; the following eleven bytes [2605,2610) are data.
The former ledger bound/decoded/instructions were 84/83/34.

- Executable body SHA-256:
  4921365d3d512d870ac24085a63d408b38ab60e6f98e4136f583a7c5c78fe753.
- Excluded tail SHA-256:
  095a90f713555df4530f7b65abc167913bbd989a9ac89489bc059ee02adf516d.

Both hashes were rechecked against the pinned original media. The existing
executable_end guard validates this body before feature decoding. Original
bytes and tail text remain private. Regeneration changes exactly this one
row's bound, decoded bytes, instruction count and ES-reference count (3 to 1).
Every other ledger field and all eight earlier span pins remain unchanged.
The 35 related queue regressions pass, including hash/bounds rejection and
tail-exclusion checks.

The reviewed baseline is main 83306859, with 229 strict implementations and
275 verified identities after #1127. This metadata repair changes neither
count nor its acceptance records. U38 +25BC is already strict and therefore
excluded from the non-strict queue. Two frozen no-git schema-9 exports agree;
the 465-candidate, 63-composable ready-ID set is unchanged from this baseline.
Classification, strict maps, reviewed identities, accounting and recovered C
are not edited. Historical snapshots above retain their original totals.

Recall reused Codex session 01a1386e-bea7-7883-8ddd-06fd2be8db9b for the
73-byte/29-instruction prefix and eleven-byte tail distinction; the linked
accepted evidence and original-byte recheck supply the hashes here.

## U7D +047A follow-up

VP instruction i19018 applies the already reviewed executable span from
[the raw +047A evidence](U7D_UPDATE_047A_CPU.md) to ledger generation.
The body is U7D [047A,0563), GAME file [148038,148271): **233 bytes /
92 instructions**, ending with RETF 4 at +0560. The two bytes [0563,0565)
are trailing data, before the next prologue at +0565.

- Executable body SHA-256:
  471d0464b3c216b69f1c3188b7a8aa2cc69ca1f81109de5217e3f4dd574da8b7.
- Excluded two-byte tail SHA-256:
  44c808fd166dbb8961fb795587e5ee0082f8a2dc781ff06a3f02223dcc06a7da.
- Historical 235-byte span SHA-256 (body plus tail):
  0c687ca966432b6c241673264978796173f6f31e91905a8a67ee3dee5110bc4e.

The body and tail hashes were rechecked against the pinned original GAME.OVR;
all 92 instructions decode contiguously through the final return. The existing
hash-checked executable_end mechanism excludes the tail before feature decoding.
Regeneration changes exactly one row: bound 235 to 233, decoded bytes 235 to
233, instructions 93 to 92. All other fields and the nine prior pins remain
unchanged. Thirty-five queue regressions pass, including hash/bounds rejection
and tail exclusion before decoding. Original bytes remain private.

On tested main 25edf536 (reviewer-owned checkpoint 235 strict / 281 verified),
+047A is already strict and excluded from the non-strict candidate queue.
Two frozen schema-9 no-git exports are byte-identical; the baseline's 459
candidates, 57 composable rows and ready-ID set are unchanged. This is extent
metadata only: no recovered C, classification, strict map, reviewed identity,
manifest or accounting edits, and no additional credit.

Dunking Sheep recall located prior +047A context in Claude session
281c8700-5acc-438b-b93d-c061e28437eb. The exact executable/tail distinction and
hashes reuse the linked accepted evidence and were independently remeasured here.

## U9E +115C follow-up

VP instruction i19057 authorizes this extent-only repair after the read-only
caller-identity feasibility report i19056. The executable body is U9E
**[115C,1287), 299 bytes / 109 reachable instruction starts**, GAME file
[180294,180593). RETF 12 starts at +1284. Every executable byte belongs to
the contiguous decoded body; its direct branches remain inside that body.
Public entry 18 maps to +115C through original stub 00B0:007A.

The excluded **[1287,1344)** suffix is 189 bytes, GAME file
[180593,180782). It contains twelve length-prefixed printable strings and
one 32-byte set at [131A,133A). Each of these thirteen data ranges has an
explicit immediate-pointer reference in the following +1344 routine; none
is a public entry. This data-use evidence supports the boundary independently
of finding an early return. Original strings and bytes remain private.

- Executable body SHA-256:
  e122e18cccee13655b750e38c4d5e6eb664fca16b125cc72bcd724580af6fb8b.
- Excluded 189-byte tail SHA-256:
  7568302f3356c70ea6504a3bb5c338a78b9c5ad02c9b5d7042ed71d2cb024550.
- Historical 488-byte span SHA-256:
  6f044b002833362a842abe523b35c28094538a592098b2635793404dcf589c4c.

All hashes were checked against the pinned GAME.OVR. The existing
executable_end guard checks the exact body hash and candidate bounds before
feature extraction. Regeneration changes exactly one ledger row: bound
488 to 299, decoded bytes 486 to 299, instructions 189 to 109, and ES
references 26 to 14. All other fields, rows and ten earlier pins remain
unchanged. Thirty-six queue regressions pass, including hash/bounds rejection
and exclusion of trailing data before decoding.

The tested baseline is main 4c83524a. Identity feasibility is separate from
accepted identity or strict C: +115C remains game_evidenced and blocked on
RES_00BA_0BB8. No implementation, classification, strict map, reviewed-identity
record, partial-callee manifest or accounting changes are included.
Two frozen schema-9 no-git exports are byte-identical. All readiness records
and the ready-ID set are unchanged: 459 candidates and 57 composable bodies.

This repair reuses the private original-byte feasibility audit from i19056
and the existing ledger-generation mechanism from #1139. Dunking Sheep recall
located earlier extent-repair context in Codex sessions
01a147c2-7af9-7ac2-98fd-eb8ba607789d and
01a147c2-b9dc-70f3-a04b-d8e47ee1e6e8; their other routine spans are not used
as evidence for this body. This body's boundary, data references and hashes
were independently rechecked against the original media.

## Newly composable bitmap callers

VP i19257 requests decoded extents for the two callers unlocked by accepted
resident +2595. These hash-pinned corrections occur before ledger decoding:

| ID | Callable span | Correct bytes / starts | Former bound | Excluded tail |
| --- | --- | --- | --- | --- |
| OVL_03B9_048F | [048F,04F8), RETF4 at04F5 | 105 / 41 | 106 | 1 byte |
| OVL_03CE_014E | [014E,01B0), RETF2 at01AD | 98 / 40 | 122 | 24 bytes |

Body hashes are respectively
08c8d4e1d889b5373c9d85613dd0530f4485892ee29681f5a14efcbfb203dcde and
244d2bbd214ceb86d9d6daa80252da6af77fa0a92e4a287ac555aaca26b3c047.
Both are checked against GAME.OVR, never copied from a queue size.
U3B9 evidence is in U3B9_BITMAP_SET_RELEASE_048F_CPU.md. The U3CE extent
repair alone offers no implementation or credit; it prepares that assigned
next target. Classification and acceptance manifests remain unchanged.

The regenerated summary also refreshes a stale decode-short total. The CSV
changes are confined to the two rows above; prior extent pins remain intact.

## Identity-cleared rows: trailing data (2026-10-20)

Found while pricing the identity-cleared rows
([IDENTITY_CLEARED_PRICING.md](IDENTITY_CLEARED_PRICING.md)). VP i19611
asked for 0511 to be recorded; the same check found two more. **Evidence and
proposed pins only.** This change regenerates no ledger, queue product,
classification or manifest; applying `executable_end` pins is the review
lane's step.

In each case the body decodes straight to its first return, with no direct
branch past it and no indirect transfer in the body. The excluded tail is
wholly length-prefixed printable strings. Whether neighbouring code
references those strings was not traced. Strings are summarised only.

| Row | Executable body | GAME file | Excluded tail | Queue extent |
| --- | --- | --- | --- | --- |
| OVL_0025_0511 | [0511,0544) 51 B / 21, RETF 4 at +0541 | [79E4,7A17) | [0544,054E) 10 B: two strings (5 and 3 chars) | 61 B / 27 |
| OVL_006F_0482 | [0482,0674) 498 B / 183, RETF 2 at +0671 | [1E468,1E65A) | [0674,069E) 42 B: seven strings (file suffixes, a disk prompt) | 540 B / 199 |
| OVL_006F_0010 | [0010,0197) 391 B / 147, RETF 4 at +0194 | [1DFF6,1E17D) | [0197,01B8) 33 B: three strings (a file name, a disk prompt) | 424 B / 160 |

SHA-256 (checked against the pinned GAME.OVR `872964a3…`):

- OVL_0025_0511:
  - body `755c5d1d6e77aaa70982d05d3c9b0b9b985fe19beac8387ecc94a9d8070c2500`
  - tail `7b286a6bb2319cfcc858802e9f5a5e162ce44e1b23e697ed93e8561b0e9e1d52`
  - historical 61 B `953309b30280c5b8ec87b1e836cc159f4ae37a9f117ed6739075bc7a6fe45f0d`
- OVL_006F_0482:
  - body `be72ede6980e67ba2ef0d51e84c154a281ab9b27b9d00070b7701fcf4bd0a4a3`
  - tail `8ccc8f762faf22b781b85f7460f4b03870e121f535ea8fbe98ee3d8de467017f`
  - historical 540 B `a939d6fc0f5514a7efd9e9a08cbff48ef14ff41eddd13652284fd381489240aa`
- OVL_006F_0010:
  - body `3dd22f1389f59ad3481634a8cb455521b7aa619a991fe1fb8b32d71171d7cb36`
  - tail `fb3e54559f7567e2ffde8d129525c2c85eddbede8c2eca93d5fb7bc39bab70c9`
  - historical 424 B `5b0d3e3a06d8837a505dcebee1d889e35c58eeff08947c4f700353eda4e80971`

Readiness is unaffected: none of the three changes its callee set, and none
becomes composable. Two further 32-byte non-text tails, on `OVL_009E_0026`
and `OVL_006F_1262`, are **not** proposed: their data use is unproven (see
the pricing record).

## U3B0 +01DA trailing data (483 B queue row, 419 B executable)

Author finding from the original bytes (GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`), recorded
with the `+01DA` re-audit; unit `0x3B0`, offsets unit-relative.

| Item | Value |
| --- | --- |
| Executable span | `[01DA,037D)`, **419 bytes / 172 instructions**, one RETF (`+037A`, `RETF 4`) |
| Span SHA-256 | `8676c97ae6dcaa88c9f7d68d75c75f81e4936572b517bd84a7e19876c58d96ce` |
| Queue / ledger bound | `[01DA,03BD)`, 483 bytes / 176 instructions (4 instructions decoded from data) |
| Excluded tail | `[037D,03BD)`, **64 bytes**, SHA-256 `b8335a5106a701003106f2a2e07b0556fb8a9eac1fa8dd86976d8ccabe0f9b2f`; 0 of 64 bytes printable (not text) |

Why the tail is data, not merely "after an early RETF": no instruction inside
`[01DA,037D)` branches or calls to or past `+037D`, and none loads an offset in
`[037D,03BD)`. The tail is two 32-byte SYSTEM set constants used by the **next**
routine: `+03BD` loads `CS:037D` at `+061C` and `CS:039D` at `+069B`, each as
`MOV DI,imm; PUSH CS; PUSH DI; LCALL 0AF8:0DC0` (SET IN). `+01DA` follows the same
convention with its own constant at `CS:01BA`, loaded at `+028A` before the call
at `+028F`. The next prologue (`55 89 E5 81`) begins at `+03BD`. The 64 bytes
themselves are not published.

`EXECUTABLE_SPANS` in `function_ledger.py` gains one hash-bound entry. The
generator rejects a different body or a bound that cuts through the reviewed span,
as for the earlier entries.

The checked-in `overlay_function_ledger.csv` changes by **exactly one row**
(authorized by the VP; extent fields only, as in the earlier repair `b1fbb101`).
Regenerating with the pinned inputs (`inventory.json`, `callgraph.json`, `near.json`,
GAME.OVR and START.EXE, under the current generator) reproduces the checked-in file
except for this row; the old and new files have the same header, the same 673 data rows
in the same order, and differ as sets by one row each:

| | bound | decoded | insns |
| --- | ---: | ---: | ---: |
| `0x3b0:0x01da` before | 483 | 427 | 176 |
| `0x3b0:0x01da` after | 419 | 419 | 172 |

Every other column of that row (grade `B`, class `glue`, far/near call counts,
`ds_globals`, `es_refs`, `ret`) is unchanged, and no classification, credit or
manifest file is touched. The generated summary
`overlay_function_ledger_summary.md` would change by one derived counter (functions
whose linear decode stops before the next prologue, 162 to 161); that file is **not**
edited here and is left for the reviewer.


## U6C +3291 trailing data (342 B ledger row, 326 B executable)

Author finding from the original bytes (GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`), recorded
2026-10-20 by par-c on the VP's instruction (i19795, job 2). The body is already
strict- and identity-credited at 326 bytes (identity row, acceptance PR #1207); this
repair brings the ledger row into line with the credited span and changes no credit.
Unit `0x6C`, offsets unit-relative.

| Item | Value |
| --- | --- |
| Executable span | `[3291,33D7)`, **326 bytes / 95 instructions**, one RETF (`+33D4`, `RETF 4`) |
| Span SHA-256 | `1e2161950d386c5df7394667e5d6079e692e551fc7610f1582ef06157528e74f` (equals the identity row's `body_sha256`) |
| Ledger bound before | `[3291,33E7)`, 342 bytes / 100 instructions (339 bytes decoded) |
| Excluded tail | `[33D7,33E7)`, 16 bytes `00 0E "Head Body Keep"` (a Pascal string), SHA-256 `ae794194e8f8761497e9e1d18fba40bf96a7d2bfad78e460ce39a29e2d170ee7` |

`EXECUTABLE_SPANS` in `function_ledger.py` gains one hash-bound entry
(`(0x6C, 0x3291): (0x33D7, 1e216195...)`); the generator rejects a different body or a
bound that cuts through the reviewed span.

The checked-in `overlay_function_ledger.csv` changes by **exactly one row**, and the
regenerated file (pinned `inventory.json`, `callgraph.json`, `near.json`, GAME.OVR and
START.EXE, current generator) equals the new checked-in file byte for byte. The header
and the other 672 rows are unchanged.

| | bound | decoded | insns | `ds_globals` |
| --- | ---: | ---: | ---: | ---: |
| `0x6c:0x3291` before | 342 | 339 | 100 | 6 |
| `0x6c:0x3291` after | 326 | 326 | 95 | 5 |

Unlike the 01DA repair, one derived feature moved besides the extent fields:
`ds_globals` 6 to 5, because one DS reference was decoded from the trailing string
bytes. Every other column (grade `A`, class `state`, call counts, `es_refs`, `ret`) is
unchanged.

**Consequences, stated up front.**
- `overlay_function_ledger_summary.md` changes by one derived counter (functions whose
  linear decode stops before the next prologue: 161 to 160); it is regenerated, not
  hand-edited.
- The ledger is a pinned input, so the regenerated queue hash moves:
  `aa3bad916a80...` to `1e5dd48106c8...`. `function_accounting_checkpoint.json` is
  re-pinned in the same PR: only `queue_sha256` and `artifacts_sha256.ledger`
  (`d38e4efe...` to `6a4bd0a5...`) change. Classification, full_c_map,
  reviewed_identities, inventory, near_calls and resident_audit still match their
  pins; strict 252 (226/26), identities 298 (271/27), unknown 511 and 898 originals
  are unchanged. The canonical `function_queue_audit.py` exits 0 in two independent
  runs with identical queue and repeat exports.
- `STRICT_CANDIDATE_QUEUE.md` is a rendered snapshot (it still lists this credited row
  at 342) and is not regenerated here.
