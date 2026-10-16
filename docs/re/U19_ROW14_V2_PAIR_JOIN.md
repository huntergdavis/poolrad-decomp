# U19 row-14 v2 source-only pair join

This separate opt-in adapter joins one audited selected-PC recorder event to
one supplied, validated four-plane packet. It leaves the published cap-128
route join and version-1 passive packet unchanged. A bounded `deja` query for
the v2 pair join found no indexed prior session; this design reuses the
checked-in candidate coverage gate, selected-PC recorder, and published
version-1 same-run join instead.

`join_pair_v2` takes the actual version-1 raw JSONL, manifest, and supplied
source bytes; the version-2 producer envelope; and the actual version-2 plane
packet bytes. p5's `audit_packet` recomputes the canonical recorder stream and
requires the envelope to match it. pR's `parse_capture_v2` bounds and parses
the plane packet bytes, rejects duplicate keys, and checks one fixed
`A000:118F` four-plane word, the selected CPU frame, state-copy equality,
and independent backing index. The join compares every selected field:
`run_token`, `arm_token`, `stream_id`, `plane_event_id`, `raw_sha256`,
`candidate_ordinal`, `pc`, and `successor_pc`.

`candidate_ordinal` counts every supplied candidate-PC visit after the U19
printer call, including other `ES:DI` destinations. The immediate successor
does not increment it. The conditional first target can occur at ordinal 897
or 898; these are source-model cases, not measured native stops. The result
retains p5's source hash, counts, terminal, and distinct full-return versus
selected-prefix status. A selected prefix never becomes a complete route.

The result is `SOURCE_ONLY_V2_PAIR_NO_NATIVE_PLANE_OR_RGB_ASSERTION` and carries
`native_coverage_proved=False`. It preserves the supplied four-plane bytes
without asserting that a native writer produced them. The recorder source
hash is recomputed against caller-supplied source bytes; this join does not
independently authenticate that source file. No host clock, page, first
writer, RGB comparison, or full-frame property is established.
