# U19 selected-PC v2 source-only producer adapter

This adapter consumes supplied version-1 raw JSONL, its manifest, and the
actual pinned debugger source bytes. It has no emulator hook, file writer,
native instruction coverage, plane capture, page/RGB result, C parity, or
full-frame claim. It is disabled unless `enabled=True` is passed as the
exact Python Boolean. The version-1 recorder and legacy cap-128 join are
unchanged.

A bounded `deja "Poolrad U19 selected PC v2 producer adapter
audit_recording actual raw manifest source eight identity provenance"`
query found no indexed session match. This implementation reuses the
merged [version-1 recorder](U19_ROW14_OPTIN_RECORDER_FEASIBILITY.md),
[2,304-candidate capacity gate](U19_ROW14_CANDIDATE_COVERAGE_GATE.md),
and [v2 alignment screen](U19_ROW14_V2_PRODUCER_ALIGNMENT.md). The
conditional first `A000:118F` candidate remains ordinal 897 or 898,
according to page order; neither is an observed native visit.

## Exact source-only output

`produce_packet` calls the merged `audit_recording` on the actual raw,
manifest, and source bytes **before** selecting data. It accepts only
`SOURCE_ONLY_FULL_RETURN_UNAUTHENTICATED` or
`SOURCE_ONLY_SELECTED_PREFIX_UNAUTHENTICATED`. A prefix keeps its own
terminal/status and never becomes a full return. The no-target, overflow,
truncated, malformed, and tampered v1 transcripts fail closed.

It emits one in-memory object, at most **4,096 bytes** in compact JSON:

| Top-level field | Type and source |
| --- | --- |
| `version` | Exact integer `2`; Boolean, float, and string are rejected. |
| `status` | One of the two unauthenticated v1 selection statuses above. |
| `selected_event` | Exactly the eight audited v1 fields: `run_token`, `arm_token`, `stream_id`, `plane_event_id`, `raw_sha256`, `candidate_ordinal`, `pc`, `successor_pc`. |
| `provenance` | Exact `source_domain`, `source_sha256`, `raw_sha256`, `candidate_count`, `frame_count`, `terminal`, `native_coverage_proved` from the audited v1 manifest. The last field remains exactly false. |

The four tokens are 1..64 characters in `[A-Za-z0-9_-]`; hashes are
exactly 64 lowercase hexadecimal digits; the candidate ordinal is an
exact integer 1..2304; PC words are canonical uppercase
`WORD:WORD`. The successor is a separate nonincrementing CPU frame.
The merged v1 bounds remain 2,307 JSONL frames of at most 256 bytes
each and a 4,096-byte v1 manifest. The v2 envelope is one distinct
4,096-byte object. These bounds do not authorize a changed pR packet.

`audit_packet` independently rebuilds the expected object through the
v1 auditor, then rejects missing/extra fields, wrong types, over-size
objects, and any mismatch in the eight selected fields or provenance.
It returns only the source-only unauthenticated status. Its input and
output are in-memory values; no original bytes or decoded media are
written into the public tree.

The pR passive four-plane packet and pC join are separate owner APIs.
They must agree on all eight selected identity fields before association.
The pR packet need not duplicate the producer's source hash or terminal.
Its exact field placement, byte cap, one-byte four-plane samples, map,
and observation-state policy require their own owner gate; this adapter
does not validate or synthesize those effects. No host clock or
authenticated lifecycle is inferred.

Synthetic checks cover both accepted terminals, first-target ordinals
897/898, default-off behavior, v1 stop classes, all eight identity
mismatches, missing/extra fields, exact type and canonical-format
rejects, 4,096-byte cap, provenance mismatch, and raw/source/manifest
tampering:

```sh
python3 -m unittest \
  tests.replay.test_cold_start_codeword_row14_v2_producer_adapter \
  tests.replay.test_cold_start_codeword_row14_selected_pc_recorder \
  tests.replay.test_cold_start_codeword_row14_selected_pc_negative_controls
```
