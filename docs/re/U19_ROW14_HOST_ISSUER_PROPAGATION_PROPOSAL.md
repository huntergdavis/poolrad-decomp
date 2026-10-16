# U19 row-14 common host issuer and propagation proposal

This is a source-only proposal for correlating one selected host write with a
later backend outcome. It installs no clock, hook, event packet, or emulator
API, and it authenticates no native frame. Two bounded `deja` searches for
the U19 event clock and backend dispatch returned no indexed finding. The
reused checked-in leads are the [field alignment](U19_ROW14_EVENT_CLOCK_FIELD_ALIGNMENT_SOURCE.md),
[handler phase-clock proposal](U19_ROW14_HANDLER_PHASE_CLOCK_PROPOSAL.md),
[backend completion boundary](U19_ROW14_BACKEND_COMPLETION_SOURCE_BOUNDARY.md),
[supplied RGB24 packet](U19_ROW14_COMPLETED_BACKEND_FRAME_PACKET_V2.md), and
[typed visual witness](U19_VISUAL_WITNESS_QA_CONTRACT.md).

## Source lock and reached boundaries

The ignored DOSBox-X source is revision
`5a9c7c53481c2e933948039c33256776653f4faf`. The merged p5 handler
header below is from public main `134a2e347f592009b9055e683ff3b81552b3fa9e`.
These are complete inclusive line windows, retaining original newlines:

| Source window | Bytes | SHA-256 | Boundary |
| --- | ---: | --- | --- |
| `tools/replay/u19_row14_in_handler_capture.hpp:100–152` | 2,156 | `6ee1eca8d64dd1b918327ac19b80db255da038de5f0283fbc59f2f5b3ee751b8` | `observe_write` samples before, calls opaque `apply_write` once at line 138, then samples after; it does not expose the interior assignment instant. |
| `tools/replay/u19_row14_in_handler_capture.hpp:155–166` | 541 | `7ac70e64e2ea25f4cfae1c08078ac1ff5e6d52f8515f6e56b02bc656acea0a9c` | `finish` checks the paired successor after the write. |
| `tools/replay/u19_row14_in_handler_capture.hpp:254–278` | 1,394 | `4ea2680807997f1c5b5ffd4c6bcbfa623ed9b55b4e289d5374e8314e492ee691` | Only later does `emit_selected_event` bind audited selected labels and derive the changed-plane mask. |
| `vga_memory.cpp:344–384` | 1,531 | `0510d93e6333a1cc461a6d9d3a5cf65cf3290289ffb82697dcede74771c0c0ba` | The generic planar path assigns the resolved backing word at line 383. |
| `vga_memory.cpp:636–649` | 656 | `f0b0dcad5958fb49f1082c6e10edae72e581070a813f488b4cb293ac47a115c5` | The fast planar path assigns the resolved backing word at lines 647–648. |
| `vga_draw.cpp:4089–4148` | 1,948 | `86621d8c607b3c6440eb2f044445af83ae5a3ee88d07e030450620131662e9ce` | The latch may complete the old frame at line 4122 before assigning new `real_start` at 4128. |
| `vga_draw.cpp:6190–6220` | 825 | `7f576f607defb5f76e47c1a62b3cf5d6a4d6adf56681f0e21a08f49150bdf74b` | The timer ends an update at 6212; line 6216 may return without a new renderer start. |
| `render.cpp:472–525` | 2,154 | `c5285bcffae637e87351d8b840652c4ecd968e11846eb9c07d1515358baf59b3` | `RENDER_EndUpdate` can return or skip graphics output; it passes only abort/changed-line state to `GFX_EndUpdate`. |
| `sdlmain.cpp:3187–3270` | 2,256 | `c705ac880a394a0e917bfd3d4d60224fe4f6aff2e946b13168d936f7b3e1d50e` | `GFX_EndUpdate` has early returns, then selects the surface or OpenGL backend. |

The handler header's full-file SHA-256 is
`b81343bcf632298da70517c1e24a6feeaed375b2888a68d476ca9f82adc4c055`.
The ignored `vga_memory.cpp` full-file SHA-256 is
`f973a04e9f5ae82864bd7699200652d123e999954b2968786d0c5b1229195374`.
The corresponding full DOSBox-X file hashes are pinned in the
[backend completion boundary](U19_ROW14_BACKEND_COMPLETION_SOURCE_BOUNDARY.md).
Neither the existing C++ handler receipt nor these native function signatures
contain a live common event issuer or `backend_event_id` source. The two
backing assignments are candidate interior hook sites; this source screen
does not select the runtime handler for a saved U19 run.

## Proposed issuer record, not an existing API

A future single host issuer would assign a strictly increasing `event_index`
within one named `clock_origin` and `clock_domain` for one `run_token`.
The origin identifies the collector instance; the domain identifies the
counter namespace. An optional monotonic host-time sample may aid diagnosis,
but its numeric value, guest CPU frame index, PIC ticks, candidate ordinal,
OpenGL frame counter, and plane-pair sequence cannot substitute for this
issuer index. An index from another origin, domain, or run must not be joined.

The proposed event record needs an event kind and phase, plus a provisional
local write reference for the selected handler operation. A wrapper-level
issuer could stamp `before_sample`, `apply_call_enter`, `apply_call_exit`, and
`after_sample` around the one opaque `apply_write` callback. These indices
bound a call interval; none is the backing-word assignment instant. A separate
`actual_store` issuer index would need an independently sourced hook at the
resolved backing-word assignment, such as the generic or fast candidate
above, with runtime handler selection proved. An explicitly reviewed
interval-only consumer contract is the other bounded option. A failed
observation retains only stamps actually issued and cannot mint an accepted
selected receipt. The raw recorder SHA-256, candidate ordinal, and complete
eight-label selected identity are bound **later**, after the
successor/`finish` and audited `emit_selected_event` result. Receipt
emission time is not relabeled as the write time. A versioned link from the
provisional reference to those eight labels would require separate p5/pC
review; no current packet has it.

The same issuer would stamp latch entry, any preassignment old-frame
completion, assignment of `real_start`, attempted new-frame start, renderer
end, selected backend dispatch, backend outcome, and later RGB observation.
A frame reference is provisional until a new renderer start succeeds; it must
retain the `real_start` actually used by that frame. The completion at
`vga_draw.cpp:4122` belongs to the previous frame, even if a new start is
assigned at 4128. `RENDER_EndUpdate` entry, abort, disabled rendering, skipped
output, `GFX_EndUpdate` early returns, and OpenGL's counter-only return at
`sdlmain.cpp:3230–3231` cannot create a new-frame completion.

A separately sourced `backend_event_id` may name a backend outcome only after
the selected branch and outcome are known. It is distinct from the selected
write's `plane_event_id`, provisional write reference, frame reference, and
issuer index. An attempted call with an ignored or void result remains
`attempt_unverified`; it cannot become `completion_verified` merely because
the call returned. The separate backend outcome proposal owns branch-specific
classification and any future independent completion criterion.

## Future join and ownership gate

A future consumer must check same run, origin and domain; strictly increasing
indices for the observed phases; the resolved eight-label selected identity;
and the actual frame reference across renderer, backend and RGB records. It
must preserve the observed wrapper-interval/latch order rather than impose
one from another run. If the latch falls inside the apply-call interval, the
interior store order remains unresolved without an `actual_store` hook; an
interval-only contract must not silently supply an exact store index. For a
claimed new-frame RGB witness, the consumer must reject an old-frame
completion, failed start, abort, early return, missing backend verification,
or RGB from another frame. Equal page words alone do not prove a redraw.

p5 owns the actual handler observation and any future issuer at that phase;
pC owns a versioned consumer; p9 owns this source proposal and the separate
backend outcome screen; pR owns the supplied RGB24 parser and any later
producer contract. p5 and pC cleared this one-file source-only scope through
durable inbox messages `i10105` and `i10106`; both require separate review
for a live hook or wire change. No selected page, first changed store,
authenticated native frame, RGB parity, or C parity is established.

Reproduce the source lock by checking the pinned revision and hashing each
inclusive line window with newlines preserved. Original source copies,
recorder logs, game media, plane bytes, and images remain ignored under
`resources/`; this note publishes only source locations, hashes, schema
constraints, and conditional control flow.
