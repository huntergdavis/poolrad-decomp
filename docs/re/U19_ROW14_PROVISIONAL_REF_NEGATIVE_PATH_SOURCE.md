# U19 selected provisional-reference negative-path source gate

This local source-only note maps failure and duplicate callback paths before
any provisional write reference is implemented. It changes no handler,
adapter, wire schema, test, or runtime. A bounded 12-second deja search for
the U19 assignment and callback paths returned no indexed result. It reuses
the [provisional receipt bound](U19_ROW14_PROVISIONAL_WRITE_RECEIPT_GATE.md),
[in-handler receipt bound](U19_ROW14_IN_HANDLER_RECEIPT_BOUND.md), and
[handler phase-clock proposal](U19_ROW14_HANDLER_PHASE_CLOCK_PROPOSAL.md).

The public source baseline is PR #809 head
`dd187df91a7ab313fc632ef174f7ba185f8056bb`, merged as `6c105984`.
The current handler header blob is
`e78df06fb4f31a475917fd9bf51f1a481d1511d8`; the adapter blob is
`f0accc59fb0c5aeb267034f001c9448ec82f7142`. Inclusive source windows
retain original newlines.

| Source window | Bytes | SHA-256 | Bounded role |
| --- | ---: | --- | --- |
| `tools/replay/u19_row14_in_handler_capture.hpp:66-78` | 471 | `546df977cd61e409fe91c5e35acb8a9086c911dcabe5a91f6320184451d65223` | Arm resets the `Capture` and accepts only four selected site/target frames. |
| `tools/replay/u19_row14_in_handler_capture.hpp:99-153` | 2,239 | `baff184f1fdf72cbe82536279a9e304e15c84e8af51d48ffb45cbc1f7783eda9` | Every `observe_write` path calls original `apply_write` once; capture guards and samples differ. |
| `tools/replay/u19_row14_in_handler_capture.hpp:155-167` | 564 | `246156a63469905bb82a6e54e9e0daa44b6465302fb8c432409ccfd348f88678` | Finish rejects prior failure, wrong successor, and missing or multiple writes. |
| `tools/replay/u19_row14_in_handler_capture.hpp:233-278` | 2,441 | `2a600c888de26acac713affbd5433dee2a014df56cdd7507edb6ec9a89a137ca` | Finished-capture and selected-event validation; effect mask compares complete plane bytes. |
| `tools/replay/u19_row14_producer_handler_adapter.hpp:91-133` | 2,127 | `4b69b9bafca1d007d4fa65929dc423faadd4a034e0a40f572dbb5aed961861d7` | Audited adapter rejects missing, duplicate, mismatched, or unaudited identity before emission. |

## Original callback effects and nonaccepting paths

| State or guard at `observe_write` | Original callback | Capture result | Reference consequence for a future issuer |
| --- | --- | --- | --- |
| Idle, finished, or already failed | Once | No new count or selected sample | No new accepted selected receipt. |
| Captured receives another write | Once | Count increments; failed with `duplicate_write` | Do not reuse the earlier candidate reference or accept either write as exactly one. |
| Armed but wrong target, width, handler, or backing | Once | Count increments; failed at the first guard | A guest effect may occur; no selected receipt. |
| Armed, unstable before sample or wrong before frame | Once | Failed after pre-sample | A guest effect may occur; keep only diagnostic indices actually issued. |
| Armed, unstable after sample or wrong after frame | Once before the failure | Failed after the guest effect | No accepted receipt despite a possible backing change. |
| Armed, all guards and samples valid | Once | Captured | Still provisional until exact successor and adapter checks. |

`finish` propagates a prior failure. Otherwise it requires the exact armed
successor frame and exactly one captured write. The adapter then requires
enabled output, no prior emission, a caller-supplied checked audit, complete
capture, valid packet shape, matching selected identity and source/raw
digests, and the selected PC pair. Its `checked` bit is a declaration from
the caller; this code does not itself authenticate native coverage.

An `arm` call resets `Capture`, including its count and stage. A future
external provisional reference therefore must be scoped by a named run and
arm and must not be recycled when the capture is reset, failed, duplicated,
or emitted. Wrapper apply-call entry/exit indices only bracket the opaque
original callback. A true `actual_store` index needs a reached interior
backing-assignment hook; none exists here. Rejected attempts may retain
diagnostic indices that were actually issued, but cannot bind any of the
eight accepted labels to another write or a later arm.

Only after successful `finish` and audited adapter emission could a
separately reviewed producer bind exactly `run_token`, `arm_token`,
`stream_id`, `plane_event_id`, `raw_sha256`, `candidate_ordinal`, `pc`, and
`successor_pc` to one reference. An accepted event with
`changed_planes_mask == 0` means four whole plane bytes agree; it is a
valid no-value-change effect, not a failed or absent receipt. No selected
mask establishes the route's first value-changing store.

The next gate is a reviewed, default-off synthetic lifecycle design for
failed pre/post samples, duplicate, wrong successor, unaudited adapter,
valid changed and valid zero masks, reset, and reference reuse. pC owns a
versioned consumer; p9 owns the separate common issuer/backend source; pR
owns passive plane and frame/RGB joins. Shared APIs require their review.
Status: `SOURCE_ONLY_REFERENCE_NEGATIVES`; no live issuer or native binding.
