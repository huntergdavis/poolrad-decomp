# Conditional dependency evidence: no members

**Status: contract only; zero members and no readiness effect.** Task t133
defines a narrow sibling of [the driver-internal class](DRIVER_INTERNAL_CLASS.md),
not an amendment to its membership or to the ready-helper rules in
[STRICT_CANDIDATE_QUEUE.md](STRICT_CANDIDATE_QUEUE.md#ready-helpers-t81-measured-on-6d369fe3).
No body is admitted by this document. The member table is
[conditionally_excluded_dependencies.csv](conditionally_excluded_dependencies.csv)
and contains its header only. There is no readiness consumer for that table.

The acceptance bar is the seven criteria in t133, plus the two additions
requiring a condition in every exclusion claim and the specific t100 refusal
evidence. The author proposes; an independent reviewer accepts. A green
empty-table test cannot accept a member or establish a real execution result.

## 1. Narrow membership and an explicit predicate

A future member is one **body/dependency/entry-condition/readiness-rule tuple**,
never a body-wide classification. It must name the direct call site, the body
and dependency ids, the allowed caller and entry set, and the precise frame
values, ownership, return convention and stub binding on which its predicate
depends. Express the predicate in the evidence document and as an assertion
in the original-byte checker. A prose assurance or an unobserved branch does
not establish it.

The only permitted claim has the form: **dependency D is excluded while
predicate P holds at entry to body B, for named readiness rule R only.**
Substitute the actual predicate for P in every claim, including the row's
`exclusion_statement`; a link to a condition in another sentence or column is
insufficient. This applies to summaries, review receipts and future member
proposals as well as the table. A report must preserve the condition whenever
it repeats the claim.

## 2. Falsifiable controls, executed in the media gate

Each proposal must carry both a passing baseline and an **executed failing
control**, with a reproducible command and retained receipt:

- The baseline runs the pinned original body under the stated predicate and
  records the relevant call/branch observations. It asserts that the named
  dependency is not reached within that exact entry domain.
- The control deliberately invalidates the load-bearing entry/frame/stub
  assumption and demonstrates the specific failure that the predicate guards
  against. Record the changed input or binding, reached dependency/edge, named
  assertion and failure result. A fabricated error, unrelated crash, missing
  media or merely proposed control does not count.
- A test must require that particular control failure; the enclosing test
  passes only when the intended assertion rejects the control. Record the
  baseline and control separately, with body, dependency, frame, caller
  census, stub and checker pins. Do not relabel an unexpected failure as the
  intended control result.

Future controls belong in media-dependent `tools/overlay/test_*.py` cases,
discovered by [tools/run_media_checks.py](../../tools/run_media_checks.py).
Run them through `tools/run_media_checks.sh` on the media host; that entry
point uses the heavy-job wrapper and refuses missing inputs or skipped
coverage. The acceptance receipt must show the member's baseline and control
actually executed in that gate on the reviewed head. CI without media and the
structural test below cannot supply this receipt. No such member control is
introduced or claimed to have run by this document.

## 3. Per-row CEILING: one named readiness rule

Every future row must state **CEILING: named rule R only; no closure,
identity, credit, replacement or unlock claim**. Replace R with the actual
rule and identify its consuming check. The dependency remains uncovered for
all other rules and entry states. This class cannot remove a call-graph edge,
mark a callee ready in general, turn a partial body into a complete one, or
change any completion count. At creation there is no consuming rule and no
readiness effect. A member row may name a readiness rule, but **the class by
itself satisfies none**. Every rule that names this class must appear in the
following registry with its own acceptance test; an unlisted rule cannot
consume a tuple. The rule's test must prove its exact bounded effect and
reject use outside the predicate.

<!-- CONDITIONAL_RULES:EMPTY -->
| Accepted readiness rule | Acceptance test |
| --- | --- |

The registry is empty. Adding its first entry requires independent review
of that rule and test, not merely a populated member row.

## 4. No transitive inheritance

A caller, sibling entry, composed closure or downstream consumer cannot
inherit a tuple's permission. Each use must have its own reviewed direct
tuple and predicate. A helper cannot use this class to satisfy another
helper's caller-identity requirement. Existing identity and closure rules
continue to apply independently.

## 5. Re-check triggers and required work

The pinned trigger tokens are `new caller`, `changed frame`, `changed stub`,
and `changed body pin`; the validator checks that all four remain documented.
Any new caller, changed frame or entry contract, changed stub binding, or
changed body pin suspends the tuple before it can be used. Dependency and
checker pin changes also suspend it. Rebuild
the direct-caller census and call-site mapping, rederive the predicate from
the pinned original bytes and entry contract, and rerun both baseline and
failing control in the media gate. A change to the named readiness rule also
requires that work and independent review. Retain the new pins and receipt;
do not silently update a hash or carry forward an old pass.

## 6. Fail-closed membership and empty table

Without an independently accepted row **and** its current executed failing
control, the dependency is uncovered. A missing, stale, skipped or
unreproducible receipt likewise leaves it uncovered. The initial table has
zero rows, like the initial `READY_HELPER_MEMBERS` set; it grants no
permissions. The structural validator rejects every row at this stage,
including a plausible-looking declaration with all fields populated.

The CSV header reserves these fields for a later reviewed implementation:

| Fields | Required meaning for a future proposal |
| --- | --- |
| `body_id`, `dependency_id`, `call_site` | One direct relationship, with original-byte provenance. |
| `entry_condition`, `readiness_rule` | Exact caller/entry/frame predicate and one named consuming rule. |
| `exclusion_statement` | The dependency is excluded while the row's actual entry predicate holds, for that named rule only. |
| `ceiling` | The explicit CEILING from section 3, repeated in the row. |
| `evidence_doc`, `control_command`, `control_receipt` | Public evidence link, reproducible media command, and retained baseline/control receipt with artifact hash and result. |
| `body_sha256`, `dependency_sha256`, `frame_pin`, `stub_pin`, `caller_census_pin`, `checker_pin` | Exact original and contract/census/checker versions; an absent stub must be recorded explicitly. |
| `review_pr`, `reviewed_commit` | Independent acceptance of those exact inputs and results. |

Before any member can land, a separate reviewed change must implement the
named rule, validate these fields and receipts, register its actual controls,
and replace the empty-only assertion with a pinned independently accepted
member set and refusal tests. Merely relaxing the empty-only test or filling
the CSV cannot activate the class.

## 7. No accounting changes

This contract adds no strict, identity, runtime, replacement, helper or driver
member. It changes no accounting input, counter, classification or readiness
implementation. Its test checks the new table and wording only. Any future
proposal must demonstrate that its named-rule CEILING leaves closure,
identity, credit, replacement and unlock claims unchanged.

## 8. Conditions stay in the claim

Use the sentence form in section 1 with the actual condition written out.
The wording guard rejects the standalone classification word when it lacks
the immediate `while` clause; reviewers must additionally check that the
clause is the real predicate, not a placeholder or a broader paraphrase.
The guard is a syntax check, not a proof of that condition.

## 9. Standing t100 refusal: RES_03F1_0000 has no membership

The specific artifact, titled **"RES_03F1_02E8 identity: NO argument-borne
game object (t100)"**, is
[RES_03F1_02E8_IDENTITY.md](RES_03F1_02E8_IDENTITY.md#two-factors), backed by
[res03f1_02e8_identity.py](../../tools/overlay/res03f1_02e8_identity.py).
Its verdict is **"Factor 1 passes; factor 2 does not."** It records 27
relocated overlay sites and seven relocated resident sites
passing CS-literal text through a stack copy to `RES_03F1_02E8`; no accepted
caller/passed-game-object/defining-document witness was found. Game-cell
reads in the verified callers gate the call rather than supply its text.
One scalar low byte can vary at `0086:+0173`, so the evidence does **not**
say that every scalar word is constant. Its explicit limit is: **"This is an
identity refusal, not a C parity claim or a credit grant."**

The complete `03F1:[02E8,0367)` body is 127 bytes; its `RETF 10h` begins at
`0364`. Its `PUSH CS` at `0329` precedes the near call at `032A` to
`03F1:0000`. Because `02E8` lacks a passing two-factor identity, that caller
still prevents `03F1_0000` from meeting the ready-helper all-callers rule.
This conditional contract cannot waive that rule or promote the refusal
into a library-identity result.

Any re-proposal must answer this concrete caller and literal-origin evidence,
identify the exact rule it seeks to satisfy, and supply the required new
evidence under that rule. Relabeling the dependency or citing a caller's
game identity alone does not answer t100. Neither body is a member here.

The t100 artifact is a static census of enumerated direct sites, not an
execution trace, dominance proof or universal census of indirect calls.
These findings are cited from that artifact, not newly measured by this
documentation change.

## Validation and what was NOT verified

Run the media-free contract check from the repository root:

```sh
python3 -m unittest discover -s tools/overlay -p 'test_conditional_dependency_contract.py' -v
```

The read-only [validator](../../tools/overlay/conditional_dependency_contract.py)
pins empty member and readiness-rule registries. The tests check the exact
header, zero members, malformed/nonempty-table refusals, missing-control and
unlisted-rule refusals, credit-row rejection, the four triggers, and condition
wording. A scoped diff guard refuses accounting or unrelated files in a PR
that touches this initial contract. Its allowlist must be independently
reviewed before a later membership implementation broadens that scope.
Synthetic receipt flags exercise refusals only; they cannot authenticate a
real receipt or admit a candidate. No member eligibility, executed
member control, media/native behavior, C parity, closure, t100 census rerun,
or readiness effect is verified by these tests.
