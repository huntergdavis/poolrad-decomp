# U19 row-14 selected-PC v2 producer alignment

This is a source-only interface screen for a future, distinct selected-PC
producer manifest and plane-event sidecar. It provides no native capture,
first guest writer, four-plane delta, page, RGB, C parity, or full-frame
evidence. The native host slot remains held. No version-1 schema or legacy
128-hit join changes are proposed here.

## Reused evidence and current boundary

A bounded `deja "Poolrad U19 row14 v2 producer manifest sidecar run arm stream
raw hash candidate ordinal897 898 plane_event"` search found no indexed
session match. This screen reuses the merged [complete candidate capacity
gate](U19_ROW14_CANDIDATE_COVERAGE_GATE.md) and
[opt-in recorder](U19_ROW14_OPTIN_RECORDER_FEASIBILITY.md), including the
synthetic tests reviewed for PR #762. The conditional fixed-row C path has
36 cells × eight rows × four planes × two stores, or 2,304 candidate-PC
visits. The first possible `A000:118F` hit is ordinal 897 on the first
page, or 898 on the second. Neither ordinal is a measured native visit.

The merged version-1 `audit_recording(raw_jsonl, manifest, source_bytes)`
recomputes the canonical frame stream, exact raw and source SHA-256 values,
counts, selected successor, and manifest. Its only selection-bearing
results are `SOURCE_ONLY_FULL_RETURN_UNAUTHENTICATED` and
`SOURCE_ONLY_SELECTED_PREFIX_UNAUTHENTICATED`. A selected prefix does not
prove the complete route. `STOP_TARGET_PAGE_ABSENT`,
`STOP_CANDIDATE_OVERFLOW`, and `STOP_TRUNCATED` carry no selected event.
The version-1 manifest's `native_coverage_proved` field is exactly false.

The exact version-1 selected identity is:

| Field | Existing meaning |
| --- | --- |
| `run_token`, `arm_token`, `stream_id`, `plane_event_id` | Separate bounded tokens; all must agree across any future packet. |
| `raw_sha256` | SHA-256 of the complete canonical JSONL byte string, including newlines. |
| `candidate_ordinal` | One-based count of every supplied one of the four candidate PCs after the U19 `+02D8` call, including wrong `ES:DI`; the immediate successor does not increment it. |
| `pc`, `successor_pc` | Uppercase `WORD:WORD` source and immediate successor. |

The existing cap-128 `store_ordinal` and event join cannot represent
ordinal 897 or 898. Relabeling, clamping, or silently passing a v2 value
into that join would lose the selected-event identity.

## Proposed distinct adapter checks, pending packet owners

A v2 adapter should consume the **actual** version-1 raw bytes, manifest,
and pinned source bytes. It must call `audit_recording` first and reject
every result except the two unauthenticated selection-bearing statuses.
It should then compare all eight selected identity fields exactly with the
audited `selected_event`, retaining the audited `source_domain`,
`source_sha256`, raw hash, frame/candidate counts, and terminal. Do not
accept a caller-supplied status, hash, or ordinal as a substitute for that
recomputation.

The new producer manifest and selected-event sidecar need a separate exact
integer version discriminator and distinct type/API. The adapter should
reject missing or extra keys; a Boolean or floating-point version/ordinal;
source/raw hashes that are not exactly 64 lowercase hexadecimal digits;
tokens outside `[A-Za-z0-9_-]{1,64}`; noncanonical PC words; and any
run, arm, stream, plane-event, raw-hash, selected ordinal, site, or successor
mismatch. An ordinal is bounded to 1..2304, with 897/898 only conditional
first-hit values for the recorded page segments. A v2 sidecar is a CPU
source selection, not a plane-change observation.

The future pR four-plane packet must bind the same eight-field identity
and actual selected `ES:DI=A000:118F` frame. Its separate evidence must
retain exactly one before and one after byte for each of four planes,
the independent host-map/backing-index check, and unchanged observation
state. The pC join needs its own versioned path beyond the legacy cap 128.
The current packet has no measured host clock; future clocks require an
explicit phase and cannot equate latch entry with selected store. The
adapter must keep `native_coverage_proved` false and return a source-only
unauthenticated result until an independently authenticated native
collector establishes coverage.

pN confirmed these review criteria in its independent review reply;
pN owns review artifacts only. pR owns the passive plane packet and pC
owns the join. Their exact v2 packet names, field layout, and ownership
agreement are pending. This note does not edit their schemas or claim an
implemented producer.

## Separate queued negative-control gate

After the v2 interface is agreed, a disjoint synthetic gate should exercise
fixed selected-PC overflow and return controls: candidate 2,305 without
appending a 2,308th raw frame; early/missing/duplicate return; return
after selected prefix; wrong selected successor; and trailing candidate
after terminal. Each case must fail closed or retain the exact prefix
status. These tests cannot prove that a native collector saw every
instruction.
