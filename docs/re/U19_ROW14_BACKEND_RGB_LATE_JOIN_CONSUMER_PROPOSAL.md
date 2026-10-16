# U19 row-14 backend outcome, frame, and RGB late-join proposal

This is a **separate read-only** consumer design after a selected-write link
has passed. It adds no native hook, backend success test, RGB producer, packet
field, or API. Current v2 RGB is a supplied byte/manifest validator: its
`completion_kind="backend_complete"` is a declaration, not observed success.
A local `deja` lookup for this late join hit the read-only index lock; no
indexed finding was reused. The checked-in [backend completion source
boundary](U19_ROW14_BACKEND_COMPLETION_SOURCE_BOUNDARY.md), [v2 RGB
packet](U19_ROW14_COMPLETED_BACKEND_FRAME_PACKET_V2.md), and [handler
clock proposal](U19_ROW14_HANDLER_PHASE_CLOCK_PROPOSAL.md) are reused leads.
pR cleared this read-only consumer boundary in durable message `i10189`;
p9 owns backend source/classification, and pR owns RGB parser/producer.

## Exact current source boundary

The private DOSBox-X source lock is revision
`5a9c7c53481c2e933948039c33256776653f4faf`. The first four
inclusive, original-newline windows below are pinned by the checked-in
backend completion boundary; original source remains under ignored
`resources/`.

| Source window | Bytes | SHA-256 | Conditional boundary |
| --- | ---: | --- | --- |
| `vga_draw.cpp:4089–4148` | 1,948 | `86621d8c607b3c6440eb2f044445af83ae5a3ee88d07e030450620131662e9ce` | Latch callback may finish the old frame at 4122 before assigning new `real_start` at 4128. |
| `vga_draw.cpp:6190–6220` | 825 | `7f576f607defb5f76e47c1a62b3cf5d6a4d6adf56681f0e21a08f49150bdf74b` | Timer may end an update at 6212 and return at 6216 before new renderer start. |
| `render.cpp:472–525` | 2,154 | `c5285bcffae637e87351d8b840652c4ecd968e11846eb9c07d1515358baf59b3` | Renderer end can skip output or return; capture before `GFX_EndUpdate` is not presentation. |
| `sdlmain.cpp:3187–3270` | 2,256 | `c705ac880a394a0e917bfd3d4d60224fe4f6aff2e946b13168d936f7b3e1d50e` | Graphics end has early returns and branch selection; OpenGL can increment a counter without swapping. |
| `tools/replay/cold_start_codeword_row14_completed_backend_rgb24_v2.py:10–34` at main `b77bfc6` | 707 | `7daf9654c896e8c5605d1a2f2ece594c7507aee4f6abd66d52697196c3257be8` | Current packet names eight selected labels and a distinct supplied backend ID, but no frame or clock. |
| `tools/replay/cold_start_codeword_row14_completed_backend_rgb24_v2.py:72–154` at main `b77bfc6` | 4,455 | `a670b3a9c0e93fda3e1524e82f64f2cf91258bff8b0c3e13f8ffa3a1f5af0e3a` | Parser checks exact supplied v2 shape, 640×400 RGB24 layout, length, digest, and declared completion kind. |
| `tools/replay/cold_start_codeword_row14_v2_rgb_association.py:1–66` at main `b77bfc6` | 2,524 | `b4c3cfea2ca0750bbf5f02f836005395b85ad9fcb75afa9b544c578ce5f46b11` | Source-only association checks eight labels; it has no sourced frame/outcome/clock correlation. |

The RGB parser's full SHA-256 is
`9ddbee8228bc5f8009408f603631e5fc2471e75c1c32e98af5756deeade5c426`.
The checked-in backend boundary separately pins the surface and OpenGL
branches: `output_surface.cpp:643–778` (5,755 bytes,
`819f1a2838c72ef82e3dab7880c394bbf47b7b143b9ad35d2611aed3df6bb91f`)
and `output_opengl.cpp:949–1135` (7,644 bytes,
`ebdba09ceeca5eec13e9b55b7fbf833ec67367abae177313628299e06e968449`).
A reached presentation call or return does not alone prove displayed output.

## Proposed separate versioned sidecar

The input is an **accepted** provisional-write-to-eight-label link from the
separate consumer proposal, plus a future sourced frame/outcome/RGB sidecar.
Each event must use the same `run_token`, `clock_origin`, and `clock_domain`
and a strictly increasing issuer `event_index`; a token or host timestamp
alone cannot make clocks comparable. Keep `plane_event_id`, provisional
`write_ref`, frame reference, and `backend_event_id` distinct.

A new-frame record needs a stable `frame_ref`, latch phase, actual frame-start
`real_start`, page and palette identities, and the renderer start that
accepted that frame. A backend outcome record needs that same frame_ref,
selected backend and compile branch, render/output flags, call site, and an
explicit `no_output`, `attempt_unverified`, or `completion_verified` class.
Only an independently sourced success criterion may yield
`completion_verified`; an ignored return, void swap, texture upload, or
`GFX_EndUpdate` entry cannot. A later RGB observation needs that same
frame_ref and page/palette/conversion identity, and must bind the actual
canonical RGB24 bytes and SHA-256 checked by pR's existing parser. The
current v2 manifest rejects frame, `real_start`, palette, phase, and clock
extras, so this would be a new reviewed sidecar, not a silent v2 extension.

For a conservative interval-only selected-write relation, require
`apply_call_exit < latch_enter` under one issuer before claiming that the
selected operation preceded the latch. An independently verified interior
`actual_store` index could support a more precise relation after separate
review. A latch inside the opaque apply-call interval has unresolved store
order; receipt emission time, CPU ordinal, packet sequence, and RGB hash do
not resolve it. After a valid new start, require sourced renderer output,
verified backend outcome, and RGB observation in that same frame with
increasing indices. The preassignment completion at `vga_draw.cpp:4122`
belongs to the previous frame even if a new `real_start` is assigned later.

The consumer must reject an old-frame completion, missing/failed new start,
timer return without start, disabled/aborted renderer, skipped graphics
output, early backend return, no-change/zero-rectangle presentation path,
counter-only OpenGL path, `no_output` or `attempt_unverified`, unsourced
`backend_event_id`, duplicate or mixed issuer indices, mismatched run/origin/
domain, and RGB with a different frame_ref, page, palette, `real_start`,
layout, or digest. It must also reject an unbound or failed selected-write
link. Equal page words or eight matching selected labels do not prove a
completed new frame.

## Unresolved gate

The current v2 association authenticates neither backend outcome nor frame
identity. No live common issuer, branch-specific success receipt, frame/RGB
link, palette identity, or selected runtime conversion is supplied. p9's
backend classifier and pR's future producer require their own source and
runtime review; pC must review a versioned consumer before any native frame,
page, RGB parity, or C parity claim.
