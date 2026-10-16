# U19 row-14 provisional write to selected identity consumer proposal

This is a read-only design for a **future, separately versioned** consumer
sidecar. It installs no issuer, handler hook, packet field, or native API. In
particular, current v2 producer, plane, pair, and RGB packets must retain their
exact field sets. A local `deja` lookup for this join hit the read-only index
lock; no indexed session finding was reused. The checked-in [handler phase
proposal](U19_ROW14_HANDLER_PHASE_CLOCK_PROPOSAL.md), [v2 pair
join](U19_ROW14_V2_PAIR_JOIN.md), and [event field
alignment](U19_ROW14_EVENT_CLOCK_FIELD_ALIGNMENT_SOURCE.md) are the reused
leads. p5 cleared this read-only consumer boundary in durable message
`i10193`; a live handler or wire change still needs joint review.

## Exact current source boundary

The public source lock is merged main
`b77bfc6710f9ce739d1456b1e4d33e762ee5f6bb`. The windows below are
inclusive, include original newlines, and have SHA-256 of those exact bytes.

| Source window | Bytes | SHA-256 | Existing behavior |
| --- | ---: | --- | --- |
| `tools/replay/u19_row14_in_handler_capture.hpp:66–78` | 471 | `546df977cd61e409fe91c5e35acb8a9086c911dcabe5a91f6320184451d65223` | Arms the selected site and guest frame. |
| `tools/replay/u19_row14_in_handler_capture.hpp:102–153` | 2,031 | `cc868874be41ac2a9979afdb88eea9ee89beb7fc3893fd36ccff2dd4535b4b37` | `observe_write` samples before, calls opaque `apply_write` once at line 138, and samples after only on the still-valid path. Guard failures also invoke the original callback. |
| `tools/replay/u19_row14_in_handler_capture.hpp:155–167` | 564 | `246156a63469905bb82a6e54e9e0daa44b6465302fb8c432409ccfd348f88678` | `finish` requires one captured write and the immediate successor. |
| `tools/replay/u19_row14_in_handler_capture.hpp:173–200` | 947 | `9944af409df8f2fbb8fc57a80e8226e3474d9e11d9dd87f66ec945f9c8b9d7a0` | Selected input and emitted receipt hold eight labels; the emitter records a changed-plane mask. |
| `tools/replay/u19_row14_in_handler_capture.hpp:233–278` | 2,441 | `2a600c888de26acac713affbd5433dee2a014df56cdd7507edb6ec9a89a137ca` | Finished capture gates `emit_selected_event`; eight labels are copied only after that gate. |
| `tools/replay/u19_row14_producer_handler_adapter.hpp:91–133` | 2,127 | `4b69b9bafca1d007d4fa65929dc423faadd4a034e0a40f572dbb5aed961861d7` | Checks audited producer binding and exact selected PC before calling the emitter. |
| `tools/replay/cold_start_codeword_row14_v2_pair_join.py:11–96` | 3,283 | `56b3ad9d67e9cabc0905b4b9d483e5aecf106e450c028a1f5de9b17baf142cac` | Re-audits raw recorder bytes and compares all eight selected labels with the supplied plane packet; status remains source-only. |

The handler header's full SHA-256 is
`b81343bcf632298da70517c1e24a6feeaed375b2888a68d476ca9f82adc4c055`;
the adapter's is
`4ec9b25dc6c988fb87275f71ca2928ad01830ec91f6bb9e418deac8946e40bd9`.
Neither has a common event issuer, provisional reference, clock origin/domain,
or live sidecar. The current v2 pair has no host clock.

## Proposed sidecar and acceptance rule

A future issuer would allocate a provisional `write_ref` within one
`run_token`, `clock_origin`, and `clock_domain`, and stamp strictly increasing
`event_index` values for issued phases. The useful wrapper phases are
`before_sample`, `apply_call_enter`, `apply_call_exit`, and `after_sample`.
These bound the opaque callback; none is the backing-word assignment instant.
An `actual_store` index requires a separately reviewed interior backing-store
hook. An interval-only consumer may instead retain the closed apply-call
interval and decline to order any event inside it relative to the store.

Only **after** successful `finish`, audited adapter binding, and a single
successful `emit_selected_event` should a versioned binding record associate
that provisional reference with exactly the eight selected labels:
`run_token`, `arm_token`, `stream_id`, `plane_event_id`, `raw_sha256`,
`candidate_ordinal`, `pc`, and `successor_pc`. The consumer must re-audit the
raw recorder and plane bytes through the v2 pair join, compare every label by
exact type and value, and require the sidecar's run token to agree. Receipt
emission time cannot be relabeled as the write time. The changed-plane mask
may describe a valid zero-change effect only after successful emission; a
failed observation must not become a no-op receipt.

The consumer must reject a missing or duplicate reference, reference reused
for a different selected identity, absent or mixed run/origin/domain,
non-integer or duplicate indices, missing/reordered phases, failed guard,
duplicate write, wrong successor, incomplete capture, disabled or duplicate
emission, unaudited packet, wrong selected PC, raw hash mismatch, and an
unbound provisional reference. A failed observation may retain only stamps
actually issued for diagnosis; it cannot mint an accepted selected receipt.
The issuer index is not CPU candidate ordinal, backing index, packet sequence,
host nanoseconds, or `plane_event_id`.

## Unresolved gate

p5 owns handler observation and any future interior store hook; pC owns this
consumer. The source does not prove which backing assignment a saved run used,
nor a common run-wide clock or authenticated native selection. A separate
schema and implementation review must settle sidecar wire shape, issuer
uniqueness, late binding, and failure receipts before any runtime claim.
