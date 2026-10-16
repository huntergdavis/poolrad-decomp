# U19 row-14 recorder, plane and frame field alignment

This is a read-only schema audit for a future authenticated selected-store to
completed-RGB join. It supplies no new native capture, event clock, EGA plane
byte, displayed page or crop comparison. A bounded `deja` query for the U19
recorder/plane/join clock and alternate page map found no matching indexed
session. Reused public leads are the [candidate capacity gate](U19_ROW14_CANDIDATE_COVERAGE_GATE.md),
[legacy same-run event plan](U19_ROW14_SAME_RUN_EVENT_PLAN.md),
[selected-store join](U19_ROW14_SELECTED_STORE_JOIN.md), and
[typed visual witness](U19_VISUAL_WITNESS_QA_CONTRACT.md). The
[opt-in selected-PC recorder](U19_ROW14_OPTIN_RECORDER_FEASIBILITY.md) and
[compact passive plane packet](U19_ROW14_PASSIVE_HOST_SNAPSHOT.md) are now
merged as PRs #762/#763. pR cleared this read-only scope; p5 and pC owner
replies were pending at this local checkpoint.

## Pinned public field meanings

At merged main `e43336e13ab13bc61be6aeaf0b1a6a22cd6e49b5`, the public
candidate coverage module SHA-256 is
`c6ae1e7afee206f8347b98d57eaeba55f816612ceb39dc6dd7fda4e4a8cbece4`;
the legacy target filter is
`fd129a71af49c35d4c68058637a01255f90c66e29beb806ecc597186167994df`.
The plane-event, legacy event-plan, route-join and typed visual-witness module
SHA-256 digests are respectively
`e9376a8f397244864ba3f7f887f131a1e6a59daef7009fa6eaca228f18cfbb34`,
`75bd7a56c2d587511aa17b2375e5a57d8aa5d9a6fe6e14c3886daee73adbdf22`,
`ccc1ba5990a5beb0e4ae473d889005f90894fe5aaa8bf28c817ed25cee1ad1be`,
and `75565a1ff26230094b84283eb6cb9b7f68e525c9c054a6ba6ce3f156d18f385e`.
The newly merged selected-PC recorder and passive-snapshot checker hash to
`e81b3754a2710f6e8d9bc489ca53651ffc508711942889c5e6bd302fee642bc6`
and `00f13ff747be8ca34210eab2f02fb91e45c27728044f06c76636ae725922d7df`.

| Source or consumer | Existing field and bound | Meaning and current gap |
| --- | --- | --- |
| Merged opt-in selected-PC recorder | Canonical `Frame(cs,ip,es,di)` JSONL, at most 2,307 records; separate version-1 manifest with `run_token`, `arm_token`, `stream_id`, `plane_event_id`, pinned source hash, raw JSONL SHA-256, segments, terminal and selected `candidate_ordinal` with `pc/successor_pc` | The sidecar binds a supplied first selected ordinal 897 if the first page is `A000`, or 898 if only the second is `A000`. The raw four-WORD frames have no clock; the recorder has no emulator hook or authenticated native coverage. A paired selected prefix returns `SOURCE_ONLY_SELECTED_PREFIX_UNAUTHENTICATED`; a complete 2,304-candidate route plus return returns `SOURCE_ONLY_FULL_RETURN_UNAUTHENTICATED`. |
| Legacy target filter and event plan | `candidate_hits<=128`; version-1 CPU `run_token` plus at most 130 four-WORD frames | Selects one supplied store/successor pair. Its hit count cannot be relabeled as the full candidate ordinal: the earliest fixed-row witness is beyond its cap. |
| Legacy four-plane manifest | version 1, one `sequence=1` event, `pc/next_pc`, before/after RAM and four 320-byte windows per sample | `sequence` indexes supplied plane pairs. It does not identify the selected CPU candidate ordinal or provide a host event clock. Same token and file location are structural checks, not capture authentication. |
| Merged pR compact passive-snapshot checker | version-1 `store_ordinal` restricted to 1..128; one byte per plane before and after, exactly four bytes in each half | Cannot encode recorder candidate 897/898. It has `run_token`, `arm_token` and `plane_event_id`, but lacks `stream_id`, `raw_sha256` and a host event clock. It must not be silently clamped, renumbered or treated as the legacy 320-byte-window manifest. |
| Typed completed-frame witness | before/after `event_index`; frame `start_event`, `backend_event`, `plane_event`, `frame_id`; RGB `event_index`, `frame_id`; `PageIdentity` and `run_token` | Requires one monotonic run-wide event clock and `before < after < frame start < backend complete < RGB`. Its current CPU input still uses the 130-frame legacy selector. Typed indices cannot be populated from candidate ordinal, plane-pair sequence or equal token strings. |

The merged compact-snapshot source has no monotonic event-clock field. Its
four-byte pair is a different capture format from the legacy four 320-byte
windows, not a smaller sample of that manifest. pC's earlier route/join also
caps 128, so it cannot represent the fixed-row 897/898 selection. The pC phase QA is a
synthetic contract without a live hook; `latch_enter` and `selected_store`
are different phases, and a CPU-frame index is not a host event index.

## Required distinct future join

A future versioned plane packet and pC join must carry the merged p5
manifest's **same** `run_token`, `arm_token`, `stream_id`, raw recorder
`raw_sha256`, selected `candidate_ordinal`, `pc/successor_pc`, and
`plane_event_id`; the join must check the selected raw `cs:ip/es:di` frame
against pR's four-byte-per-half observation. The pC join must verify this
binding against recorder provenance rather than trusting a matching label.
Strict types should reject bool/float ordinal values,
missing/extra fields, wrong plane count or length, swapped run/arm/event/hash
or stream IDs, wrong store successor, and a selected ordinal inconsistent
with the active/draw segment case. A `897` first-page event cannot be reused
as `898` after swapping which page maps to `A000`.

That versioned selected-store join is **separate** from the completed-frame
clock. A passive collector must supply a real shared monotonic host clock
for store before/after, later new-frame start, backend completion and RGB
observation; the page identity and palette must agree on the completed
frame. An old-page completion before the display-start latch, a missing
backend completion, or an RGB record from another frame must stop. The
existing typed witness validates those supplied relationships offline but
does not authenticate their source. No current public schema maps the full
candidate stream to that host clock.

## Negative controls and release boundary

The exact stop conditions are: legacy 128/130 cap at candidate 897 or 898;
candidate stream missing, reordered or with an extra stop; full-stream
ordinal replaced by legacy `candidate_hits` or plane `sequence`; 897/898
swapped under alternate page words; mismatched selected successor; compact
four-byte packet substituted for a 320-byte-window manifest; equal labels
without matching raw recorder hash/stream/arm identity; CPU instruction
index compared to a host clock; old-page or incomplete backend frame;
different palette/page/frame ID; and missing passive-read or lifecycle
proof. The merged recorder's accepted source-only outcomes are
`SOURCE_ONLY_SELECTED_PREFIX_UNAUTHENTICATED` and
`SOURCE_ONLY_FULL_RETURN_UNAUTHENTICATED`; neither proves native coverage.

The merged p5 recorder and pR compact checker are offline source-only tools.
Only after a native collector authenticates the complete candidate stream,
pR's selected packet is versioned to bind it, a passive collector supplies a
verified shared host clock, and pC supplies the versioned join can a separate
completed-frame RGB24 crop comparator consume one run's data. That
authenticated join and comparison are not implemented or fabricated in this
note. The independent
36-cell crop SHA-256 is
`e6f630a6abf12a030e92387a2966f7ab15abff63f8a8a57b9b8988e516dff980`;
it is desired output, not a measured frame from such a packet.

Only address, hash, count and schema facts are public. Original media,
recorder logs, RAM, four-plane bytes and RGB remain ignored in `resources/`.
There is no first-writer, selected page, native RGB or C-native parity claim.
