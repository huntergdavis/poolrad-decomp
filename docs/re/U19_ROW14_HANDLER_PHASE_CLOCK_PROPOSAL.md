# U19 selected handler phase-clock source proposal

This read-only proposal names the information a future same-run handler,
backend and RGB join would need. It adds no live hook, host timestamp, packet
field, native capture, or demonstrated frame order. A bounded `deja` query
for the handler before/store/after clock origin and domain timed out after
12 seconds without an indexed result. Reused checked-in work is the
[recorder/plane/frame field alignment](U19_ROW14_EVENT_CLOCK_FIELD_ALIGNMENT_SOURCE.md),
[selected receipt bound](U19_ROW14_SELECTED_RECEIPT_EFFECT_SOURCE.md),
[same-run join contract](U19_ROW14_SAME_RUN_JOIN_CONTRACT.md), and the
[backend completion source boundary](U19_ROW14_BACKEND_COMPLETION_SOURCE_BOUNDARY.md).

## Current source boundary

At merged main `2b1655f3a970506bb048eb7edc9088c93275881a`,
the p5 handler capture header hashes to
`fe232c91f52947c5df09c62d9e16c6ea4bafe9e439ebe802fc2d4af64de42d2c`.
`observe_write` checks the selected frame and resolved backing index, reads
the before host state and four plane bytes, executes the original
`apply_write` once, then reads the after state and four bytes.
`finish` requires exactly one accepted write and the selected immediate
successor. None of `GuestState`, `Sample`, `Capture` or `SelectedEvent`
has a host clock, clock domain, origin, or backend event identifier.
The p5 producer-handler adapter hashes to
`4ec9b25dc6c988fb87275f71ca2928ad01830ec91f6bb9e418deac8946e40bd9`;
it binds the eight audited selected identity fields only after the
capture finishes. Its emit time cannot stand in for the actual store time.

The pC phase-clock QA source hashes to
`8e72c83f4061e93eefe6d7a0ce06ab3c1f4e889e9fd13136baa5671d564659ab`.
It accepts supplied `latch_enter` and `selected_store` samples with a
`host_ns` value, checks same-phase matching keys and equality, and explicitly
has no live producer hook. Its two phase labels do not directly provide
handler before-copy and after-copy samples. A versioned pC review is needed
before mapping new handler phases to that synthetic consumer.

## Future handler-side capture requirements

An eventual handler receipt would need one **named common monotonic source**,
including its origin and domain, shared with the backend producer. It would
bind each phase sample to the same audited
`run_token/arm_token/stream_id/plane_event_id/raw_sha256/candidate_ordinal`
and selected `pc/successor_pc`. The proposed distinct handler phases are:

| Phase | Boundary in existing handler structure | Current evidence |
| --- | --- | --- |
| before | Stable pre-write state and four-byte copy completed, before `apply_write` | Source execution order only; no timestamp. |
| store | Actual resolved backing-word write within the original handler operation | The wrapper calls `apply_write` once, but does not timestamp the interior write. |
| after | Stable post-write state and four-byte copy completed, before `finish` | Source execution order only; no timestamp. |

A timestamp taken when the recorder emits JSON, at `finish`, or at a UI
callback is not a timestamp for the selected store. A clock in a different
domain or with an unspecified origin cannot be ordered against backend
completion. If host timestamps may tie, a future source must also supply a
run-wide ordered event index from that same producer domain; the CPU
`candidate_ordinal`, packet `sequence`, and backing index cannot replace
it. The current code proves only the wrapper call order, not a common host
clock or when a backend frame becomes visible.

p9 has given relative CLEAR for this handler source-only proposal. Its
merged backend completion note pins conditional presentation and early-return
source sites, and explicitly does not supply a common live clock or verified
display completion. A future `backend_event_id` is a distinct field from
p5's `plane_event_id`.
Correlation requires a shared run and frame plus sourced comparable event
indices under the same named origin/domain, not equal token labels. pC owns
the consumer and phase QA; its direct API boundary review is pending.
pR owns the passive plane packet and downstream RGB/frame capture.

## Negative contract and next gate

A future versioned validator must stop on absent or mixed clock domains,
unbound run/arm/stream/raw SHA or selected PC, missing one of the three
handler phases, a store phase from another candidate, swapped phase order,
or a backend receipt whose event index is not sourced from the same
clock origin. It must keep invalid/missing handler receipts distinct from
valid no-value-change effects. No test may manufacture a native event from
`candidate_ordinal` or from an offline producer packet.

The next local gate is pC review of the named-origin/domain and phase identity
requirements. Any live hook or cross-producer schema remains a separate
measured implementation. No native writer, backend completion, displayed
page, RGB, or full-frame parity is claimed here.
