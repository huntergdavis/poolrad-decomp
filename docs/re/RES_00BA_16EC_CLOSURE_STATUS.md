# 16EC closure status (t131)

**NOT-CLOSABLE on the currently recorded evidence.** This is a disposition
of this evidence step, not a claim that the original can never be ported.
It confers no identity, readiness, membership, credit, replacement, or
unlock. Independent review remains required, and t100's NO is unchanged.

The [conditional boundary proof and Move-alias control](RES_00BA_16EC_PORT_BOUNDARY.md)
are written up separately. Under pinned original code, correct loaded
targets, and preservation of the box/tile control frames by all callees,
the fixed caller narrows the tile path to its three band-4 tiles and 92
calls. Violating preservation with the original Move reaches the real error
path after two calls; the EGA-attached C model refuses there instead of
matching the original EXIT. The one native mode-5 selector point satisfies
a box-bypass predicate only. None of these observations closes the reasons
below.

## Recorded open reasons and their falsification tests

Each table row is an open reason. Its removal test states what evidence
would remove it, subject to independent review of the named port scope.
A reason is not permanent merely because it appears in this document.

| ID | Current reason | Removal test |
| --- | --- | --- |
| NC1 | No reviewed port-wide selector/entry contract is established; one native mode-5 point is only a sample. | A reviewed port entry contract defines the admitted entries and supplies a checked invariant or coverage argument for their selector paths; a single sampled arrival is insufficient. |
| NC2 | Live box-frame preservation is unproved; the original Move-alias control shows why it is required. | For every admitted box-taking entry, a checked write-range/frame-preservation argument or enforced guard excludes the demonstrated Move alias and all other control-frame writes; alternatively the entry contract proves all box calls excluded. |
| NC3 | The remaining selected clear/title/printer and common-tail live effects have not been compared at the port boundary. | Reviewed same-entry original/C evidence validates the remaining selected clear/title/printer and common-tail effects under the port contract, or checked path evidence excludes the named callees; compare the declared CPU/RAM/display/events/stop contract. |
| NC4 | Cold loading or its exclusion at admitted entries is unresolved; a cold synthetic stop is not a successful box. | The admitted entry contract has a checked loaded-stub/target invariant or enforced guard excluding cold calls, or a reviewed original/C loader comparison establishes the required cold-call behavior. |
| NC5 | The inherited 255-byte printer/nontermination boundary has no established port entry exclusion. | A checked read-time length/argument-preservation bound excludes the 255-byte printer case for admitted entries, or the port contract and comparison explicitly validate that case's nontermination semantics without treating budget exhaustion as return. |

## Rejecting stale reasons

The companion `res16ec_closure_obligations.json` records dispositions for
these five reasons. Initially every reason is open and every discharge is
null. After an independently reviewed discharge, record its reviewer,
review reference, exact reviewed head, proof reference and SHA-256, and
applicable scope, mark that reason discharged, and remove its open table
row here. Never invent a discharge merely to make the check pass.

Run `python3 tools/overlay/res16ec_closure_status.py`. It fails if a reason
recorded as discharged is still asserted open here, an open reason is
missing, a reason ID or removal criterion differs from its companion record, or
required discharge references are absent. The unit control supplies an
explicitly synthetic discharge for each of NC1 through NC5 and requires
the stale document to fail, then requires the corrected open set to pass.
An additional control discharges all five and rejects a retained
NOT-CLOSABLE headline; removing open reasons does not itself grant closure.
The fixtures are not actual proofs or review verdicts. They are included
in `tools/run_media_checks.sh` discovery.

The focused freshness run passed six tests, zero skips (0.038 seconds),
including the five per-reason stale-discharge subcases and stale headline. Its receipt is
`resources/t131/closure-freshness.log`. These are consistency controls, not
executed discharges of the five obligations.

**Limit of this check:** it checks consistency with recorded review
metadata and reference shape. It does not discover new proofs, read inbox
verdicts, authenticate reviewers, validate referenced proof contents, or
prove a negative about unexamined evidence. An unrecorded discharge can
leave the record stale; the reviewer must reconcile new evidence first.
A freshness PASS is not a closure, acceptance, readiness, or credit verdict.
There is no connection from this record to accounting/readiness products.

## Not verified

No actual discharge of NC1 through NC5 is recorded. Native original/C
16EC parity, completion of the interrupted route, all-arrival invariants,
live box preservation, cold loading, and general termination are not
verified. The one native snapshot and synthetic comparisons have only the
scope stated in the separate boundary evidence document.
