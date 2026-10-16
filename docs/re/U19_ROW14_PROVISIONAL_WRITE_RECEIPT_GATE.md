# U19 selected provisional-write receipt source gate

This source-only gate bounds a future handler-local write reference through
a selected four-plane receipt. It adds no clock, issuer, reference field,
packet wire change, native trace, or accepted first writer. Two bounded
deja searches for actual-store and late-receipt paths timed out without
an indexed finding. Reused checked-in research is the
[handler phase-clock proposal](U19_ROW14_HANDLER_PHASE_CLOCK_PROPOSAL.md),
[handler effect field contract](U19_ROW14_HANDLER_EFFECT_FIELD_CONTRACT.md),
[selected receipt effect bound](U19_ROW14_SELECTED_RECEIPT_EFFECT_SOURCE.md),
and [v2 pair join](U19_ROW14_V2_PAIR_JOIN.md).

## Exact current handler and adapter boundary

The public baseline is main e513cad972ba58a1edbe59b91f3525a5d5e74d7f.
The in-handler header blob is e78df06fb4f31a475917fd9bf51f1a481d1511d8;
the producer-handler adapter blob is
f0accc59fb0c5aeb267034f001c9448ec82f7142. Inclusive line windows
retain original newlines.

| Public source window | Bytes | SHA-256 | Current step |
| --- | ---: | --- | --- |
| tools/replay/u19_row14_in_handler_capture.hpp:46-78 | 983 | d69e0312e0ba70ccd18313b860ebb0b65b25ea940c62d0215b82d9996ce3a81c | Capture stage and selected arm: site, immediate successor, CS:IP/ES:DI. |
| tools/replay/u19_row14_in_handler_capture.hpp:102-153 | 2,031 | cc868874be41ac2a9979afdb88eea9ee89beb7fc3893fd36ccff2dd4535b4b37 | One original opaque apply_write call after before-sample; after-sample only on still-valid path. |
| tools/replay/u19_row14_in_handler_capture.hpp:155-167 | 564 | 246156a63469905bb82a6e54e9e0daa44b6465302fb8c432409ccfd348f88678 | Failed capture, successor, captured stage, and exactly-one-write gate. |
| tools/replay/u19_row14_in_handler_capture.hpp:170-198 | 1,095 | 22c6f556c88950fb5a9987ccb4997982c25bfd2398010b161684755ceca59683 | Six supplied identity fields; PC and successor are later formatted from capture; effect mask is meaningful only when emitted. |
| tools/replay/u19_row14_in_handler_capture.hpp:233-278 | 2,441 | 2a600c888de26acac713affbd5433dee2a014df56cdd7507edb6ec9a89a137ca | Finished-capture validation, late receipt assembly, four whole-byte plane comparison, single commit. |
| tools/replay/u19_row14_producer_handler_adapter.hpp:37-53 | 771 | c7e0546b182f5e80c236ae44d582ea4fed22aab21189ddabeb95812799dde66f | Caller-supplied audit binding and distinct rejection results. |
| tools/replay/u19_row14_producer_handler_adapter.hpp:70-133 | 3,037 | 70acce9f4c09c8f407060512eb2a2a33a5ed9532daf05fcefc2aa46f7fd2c086 | Shape, audited identity, source/raw digests, and exact selected PCs checked before emitter. |

The header has no provisional reference, host clock, origin/domain,
backend identifier, or live sidecar. The adapter's checked bit and
audit data are supplied by its caller; the adapter does not itself prove
a native capture. Direct calls to the header emitter validate shape
and completion, not the external recorder audit. The existing nested
producer and flat passive v2 packets keep their exact field sets and
carry no common host clock.

## Failure and successful effect distinctions

The original apply_write callback still executes once on a wrong
target/width/handler/backing guard and on a duplicate armed write. An
unstable before-sample or wrong observed frame likewise leaves the
capture failed after the original call. A failed after-sample or frame
also rejects capture; an idle, finished, or already failed observer
passes the callback through without making a new receipt. These paths
must not promote a provisional reference to an accepted selected event.

Finish returns a prior failure; otherwise it requires the selected
immediate successor and one captured write. At the adapter, disabled,
duplicate, unchecked audit, incomplete capture, invalid packet,
audit/source/raw mismatch, or wrong selected PC returns without
emission. Only after those checks does the adapter supply six identity
values to the header emitter; the emitter formats the two selected PCs
from capture and compares complete before/after plane bytes before
setting emitted true. Invalid identity leaves emitted false.

One accepted write can have changed_planes_mask zero when all four
whole plane bytes agree. That is a valid no-value-change effect only
while emitted is true. An absent, failed, or duplicate observation
cannot be represented as that valid zero mask. The selected bit 0x20
alone cannot determine a whole-byte change, and one selected mask
cannot establish the first value-changing store in the route.

## Proposed p5-only lifecycle gate

A future handler-side issuer may allocate a provisional local write_ref
under a named run, clock origin and clock domain. Wrapper samples and
apply-call entry/exit can receive ordered indices, but these only
bracket the opaque call. An actual_store index requires a separately
reviewed interior backing-assignment hook in the reached generic or
fast handler. The local reference must survive through the original
callback and exact successor check; a failed attempt may retain only
indices actually issued for diagnosis. No failure may mint an accepted
selected receipt or reuse its reference for another identity.

Only after successful finish and audited adapter emission could a
separately versioned binding associate that reference with exactly
run_token, arm_token, stream_id, plane_event_id, raw_sha256,
candidate_ordinal, pc, and successor_pc. Candidate ordinal, backing
index, packet sequence, and emission time are not issuer indices or
store timestamps. The first small synthetic gate would exercise a
valid changed mask, valid zero mask, pre-sample failure with original
apply once, duplicate, wrong successor, invalid identity, unaudited
adapter, and reference reuse. It would assert that only one valid
finished event binds an identity. No such test or live hook is added
by this source note.

p5 owns this handler-local lifecycle. pC owns the future versioned
consumer, p9 owns the separate common issuer/backend source proposals,
and pR owns the passive plane packet plus frame/RGB join. Any wire or
shared API change requires their joint review. Status:
SOURCE_ONLY_PROVISIONAL_RECEIPT_BOUND; LIVE_CLOCK_AND_NATIVE_BINDING_UNPROVEN.
