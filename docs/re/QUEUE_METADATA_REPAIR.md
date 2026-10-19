# Queue span and indirect-dispatch metadata repair

This schema-9 hygiene change corrects four planning traps found after the
U3B0 +1140 raw upgrade. It changes queue metadata and two ledger rows; it
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
