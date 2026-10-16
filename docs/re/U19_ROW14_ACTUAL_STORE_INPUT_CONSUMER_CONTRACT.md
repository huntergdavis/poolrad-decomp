# U19 actual-store input acceptance contract for a future consumer

This read-only contract refines the merged [provisional-write consumer
proposal](U19_ROW14_PROVISIONAL_WRITE_CONSUMER_PROPOSAL.md). It specifies what
a future versioned input could prove about one selected handler operation.
It adds no issuer, hook, sidecar, packet field, parser, or native capture.
A bounded local `deja` lookup hit the read-only index lock; no indexed
session finding was reused. The checked-in [p5 provisional receipt
gate](U19_ROW14_PROVISIONAL_WRITE_RECEIPT_GATE.md) and [p9 common issuer
proposal](U19_ROW14_HOST_ISSUER_PROPAGATION_PROPOSAL.md) are reused source
leads. p5 confirmed the handler boundary in durable message `i10439`.

## Pinned source boundary

Public baseline is merged PR #810 main
`5811ca402867ddb2831ca093ff090eefe1ef15a5`. Complete inclusive line
windows retain original newlines. These are source paths, not measured
events from a saved U19 run.

| Window | Bytes | SHA-256 | Consumer implication |
| --- | ---: | --- | --- |
| `tools/replay/u19_row14_in_handler_capture.hpp:102–153` | 2,031 | `cc868874be41ac2a9979afdb88eea9ee89beb7fc3893fd36ccff2dd4535b4b37` | The original `apply_write` is opaque at line 138; wrapper enter/exit can bracket it but cannot timestamp its interior assignment. Failed paths may still call it. |
| `tools/replay/u19_row14_in_handler_capture.hpp:155–167` | 564 | `246156a63469905bb82a6e54e9e0daa44b6465302fb8c432409ccfd348f88678` | Finish requires the selected successor and exactly one captured write. |
| `tools/replay/u19_row14_in_handler_capture.hpp:233–278` | 2,441 | `2a600c888de26acac713affbd5433dee2a014df56cdd7507edb6ec9a89a137ca` | Selected identity and effect mask are committed only after finished capture; mask zero is meaningful only when emitted. |
| `tools/replay/u19_row14_producer_handler_adapter.hpp:70–133` | 3,037 | `70acce9f4c09c8f407060512eb2a2a33a5ed9532daf05fcefc2aa46f7fd2c086` | Caller-supplied audit binding, source/raw digests and selected PCs are checked before emission. |
| `tools/replay/cold_start_codeword_row14_v2_pair_join.py:45–96` | 2,512 | `1f59464f9908f67226dc47bcd39aa8c8d73eccb21bae4894ac8c038b2a310c9d` | Current source-only pair join recomputes raw recorder selection and compares all eight selected labels. |
| `vga_memory.cpp:344–384` | 1,531 | `0510d93e6333a1cc461a6d9d3a5cf65cf3290289ffb82697dcede74771c0c0ba` | Generic planar resolved backing-word assignment candidate at line 383. |
| `vga_memory.cpp:636–649` | 656 | `f0b0dcad5958fb49f1082c6e10edae72e581070a813f488b4cb293ac47a115c5` | Fast planar resolved backing-word assignment candidate at lines 647–648. |

The public handler, adapter and pair-join blobs are respectively
`e78df06fb4f31a475917fd9bf51f1a481d1511d8`,
`f0accc59fb0c5aeb267034f001c9448ec82f7142`, and
`7eba74fc302e50d8dbb40432ffb3d7d38d25c32e`. The private windows are
from ignored DOSBox-X revision `5a9c7c53481c2e933948039c33256776653f4faf`.
Neither candidate interior handler is proved reached by the saved U19 route.

## Proposed future input and evidence levels

A separately versioned sidecar would carry a provisional `write_ref` scoped
to one `run_token`, `arm_token`, named `clock_origin` and `clock_domain`.
Each issued phase has a strictly increasing integer `event_index` from that
same issuer. A successful wrapper observation would provide
`before_sample < apply_call_enter < apply_call_exit < after_sample`.
Failed observations may retain only issued diagnostic phases. The sidecar
must distinguish these evidence levels explicitly:

| Level | Minimum accepted facts | Permitted conclusion |
| --- | --- | --- |
| Diagnostic only | Failed, duplicate, idle, finished, invalid or unbound attempt, with any actually issued phases. | No accepted selected receipt or no-op effect; no causal store ordering. |
| Wrapper interval | Unique reference; successful finish; audited single emission; exact eight-label binding; all four ordered wrapper phases. | One accepted selected operation is bounded by the opaque apply-call interval. There is no exact `actual_store` index. |
| Sourced actual store | All wrapper-interval facts **plus** a separately reviewed reached generic or fast interior assignment hook, the resolved backing location and handler branch, and an `actual_store` index under the same issuer strictly between call enter and exit. | Exact interior store ordering may be considered for this operation, subject to the hook's independent source review. |

The consumer must never promote a wrapper stamp, capture emission time,
candidate ordinal, backing index, packet sequence, or host nanoseconds to
`actual_store`. If an exact-store claim is requested but the interior hook,
runtime branch, resolved location, or same-issuer index is absent, reject it;
an explicitly selected interval-only contract may still retain the bounded
operation without an exact-store claim. A latch inside the call interval
does not reveal store order. A conservative later relation can be asserted
only when `apply_call_exit < later_event_index` in the same issuer domain;
an interior store hook can support a narrower relation after review.

## Fail-closed selected binding

Only a successful `finish` followed by one audited adapter emission may
associate `write_ref` with exactly `run_token`, `arm_token`, `stream_id`,
`plane_event_id`, `raw_sha256`, `candidate_ordinal`, `pc`, and
`successor_pc`. Recompute the selected producer/plane pair from actual raw
recorder and packet bytes, then compare each label by exact type and value.
The adapter's caller-supplied checked bit alone is not native provenance.
Reject missing, duplicate, reused or cross-run/arm/origin/domain references;
non-integer or repeated indices; missing/reordered successful phases;
failed guard, wrong successor, duplicate write, incomplete capture,
disabled or duplicate emitter, unaudited packet, source/raw mismatch, wrong
selected PC, and identity rebound to another attempt. Valid emitted
`changed_planes_mask == 0` means no whole-plane-byte change for that
accepted operation; it must not encode failure or absence. Neither outcome
proves the first value-changing writer across the candidate stream.

## Independent next gate

p5 owns any handler-local issuer and interior store hook; p9 owns common
issuer provenance and runtime branch evidence; pC owns this consumer. First
independent gate is a p5/p9 source review of the proposed discriminator and
reference lifetime against the exact merged handler and private assignment
sites. A later versioned wire/API and synthetic tests need separate approval.
No live clock, actual selected store timestamp, native first-writer, RGB,
or C parity follows from this document.
