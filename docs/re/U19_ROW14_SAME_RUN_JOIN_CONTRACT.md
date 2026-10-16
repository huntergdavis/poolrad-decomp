# U19 row-14 same-run source join contract

PM inbox i8307 assigned a narrow integration boundary across p5's selected
host RGB event, pR's passive four-plane/latch snapshot, and p9's 36-cell
expected RGB map. The source correction i8332 pins `38D3:02D8` as the
printer **far call**, the selected store as one of
`0F2D:0412/0438/0445/0451` with effective `ES:DI=A000:118F`, and the
printer return at `38D3:02DD`. The row source is 36 cells and 2,304 CPU
store operands; it is not the unrelated U17 459-cell adapter. A bounded MCP
recall attempt stalled and was cancelled. This gate reuses the checked-in
eight-stop route, selected-store filter, plane event audit, and source-derived
row-14 CPU model rather than an unverified recall result.

`join_source_contract` consumes **validated results**, not unchecked capture
JSON. The route input is the existing `audit_route` result, now carrying the
run token it validated from its three private documents. The host input is
p5's `correlate_selected` result; its event log has one `run`, contiguous
0-based `seq`, monotonic `host_ns`, and a selected store carrying
`plane_event_id`, shared `arm_token`, and 1-based `store_ordinal`. The passive
input is pR's
`validate_capture` result; it carries `run_token`, `plane_event_id`,
`arm_token`, `store_ordinal`, selected site/successor, backing index and four
before/after plane bytes. The join key is exactly
`(run_token, plane_event_id, arm_token, store_ordinal)`; the route filter's 1-based
`candidate_hits` must equal that ordinal. Guest frame indices and host event
indices are separate clocks and are never compared numerically.

`store_ordinal` counts every **supplied selected-PC frame after the U19 printer
call**, including frames with the wrong `ES:DI`. It is neither a matching
`A000:118F` count nor a count of all guest writes. The present public
target filter and producer validators cap this ordinal at 128. The accepted
36-cell EGA source model has 2,304 candidate-PC executions across the fixed
printer route; a conditional exhaustive capture cannot reach its first
possible `A000:118F` witness before ordinal 897 (or 898 if only the second
page is `A000`). Thus the current adapter is a structural interface for
supplied data, not proof that the capped debugger recipe covered that route.
An authenticated sparse selection or reviewed larger-cap schema is still
required before a live same-run claim.

Two local QA probes remain synthetic. Probe A changes only the audited route
token while keeping the host and passive event key fixed and requires the
join to reject it. Probe B supplies identical host and passive common-clock
samples at `latch_enter` and `selected_store`, then substitutes a latch-tagged
sample at the store phase and requires rejection even if the number matches.
It separately rejects a store-phase clock value copied from the latch phase.
The current p5 validated witness omits latch raw clocks, and pR's validated
result has no phase-tagged common host clock. Probe B therefore has no live
producer adapter or timing claim; a reviewed hook schema must precede one.
The helper compares values only within each named phase. It imposes no
latch-versus-store order: p5's validated event sequence permits the selected
store before latch entry, while retaining its own renderer-end constraint.

The producer validators own chronology and capture-specific state:

1. Guest route: U19 `+02D8` printer call, selected drawer instruction at
   `A000:118F`, immediate paired successor, then U19 `+02DD` return.
2. Host event: `printer_call < selected_store < printer_return`; its host
   sequence requires `crtc_sample < latch_enter < latch_assign <
   renderer_start < renderer_end(output=True) < backend_complete <
   rgb_capture`, with the selected store before renderer end. The backend
   completion and RGB capture share one `backend_event_id` and frame.
   The latch event requires raw PIC ticks, CPU cycles, and frame-start time;
   these are latch-entry observations and are not equated to store or
   successor fields. An extra guest status-port read is not part of this
   contract.
3. Passive snapshot: a single hooked store with the exact successor and
   unchanged state within each direct backing-memory copy. Latch and
   controller state may change across the guest instruction; pR validates
   that packet independently.

The expected input is p9's privately generated, tightly packed RGB24 crop:
36 cells, 2,304 source pixels, host rectangle `(16,224,576,16)`, stride
1,728, and 27,648 bytes. The reviewed p9 patch has SHA-256
`e6f630a6abf12a030e92387a2966f7ab15abff63f8a8a57b9b8988e516dff980`.
Its source and font digests are supplied by the caller; this join checks their
form and records the crop digest, but does not authenticate the patch or
regenerate pixels. The typed QA wrapper can carry the rectangle as
`RGB24Patch`. This route-specific adapter requires a canonical 640×400 RGB capture,
completed backend, and validated palette SHA/generation. Equal-page
`real_start` is allowed; page words `0/0` alone cannot prove a redraw.

The returned status is `SOURCE_ONLY_JOIN_NO_PIXEL_OR_WRITE_ASSERTION`.
No first selected value-changing store, postreturn RGB difference, page
causation, native capture authenticity, whole-frame parity, or full-C credit
follows. p5's corrected local head `bd28f44` exposes the complete validated
host witness, including `arm_token`, ordinal, palette identity, and RGB
dimensions. pR's corrected local head `458a781` carries the shared
`arm_token` and validates one-byte-per-plane extent. p9's reviewed PR #742
remains a separate producer input. The protocol avoids importing unpublished
files. Synthetic tests use invented bytes only. Original media, font, RAM,
EGA planes, palette and RGB remain under ignored `resources/`.
