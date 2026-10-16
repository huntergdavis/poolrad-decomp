# U19 new-frame, backend, and RGB acceptance contract for a future consumer

This read-only contract refines the merged [backend/RGB late-join consumer
proposal](U19_ROW14_BACKEND_RGB_LATE_JOIN_CONSUMER_PROPOSAL.md). It defines
which future inputs could support one completed **new** frame linked to a
selected handler operation. It installs no collector, common clock, frame
token, backend witness, RGB conversion, packet field, parser, or native test.
A bounded local `deja` lookup hit the read-only index lock; no indexed
session finding was reused. The checked-in [pR page/crop/palette
source note](U19_ROW14_PAGE_CROP_PALETTE_CHANNEL_CONTRACT_V2.md), [p9 backend
outcome source](U19_ROW14_BACKEND_OUTCOME_CLASSIFICATION.md), and [supplied
RGB24 packet](U19_ROW14_COMPLETED_BACKEND_FRAME_PACKET_V2.md) are reused
leads. pR confirmed its conditional producer boundary in durable message
`i10445`.

## Pinned current source boundary

Public baseline is merged PR #810 main
`5811ca402867ddb2831ca093ff090eefe1ef15a5`. The original private
DOSBox-X revision is `5a9c7c53481c2e933948039c33256776653f4faf`.
Complete inclusive windows retain original newlines. They identify
candidate observation sites, not a measured saved-run frame.

| Window | Bytes | SHA-256 | Consumer implication |
| --- | ---: | --- | --- |
| `vga_draw.cpp:4119–4148` | 1,103 | `bffc6f2bee77aff833ea09a015384a2700ae3f5b266a3673de1f761e773c4000` | The latch may complete the old frame before assigning the new frame's `real_start`; pending `display_start` is not the accepted frame page. |
| `vga_draw.cpp:6259–6290` | 1,151 | `888d758087aa016fa34a920fd9913b6393723e920c22478c128ad5f3e3e9b9a9` | EGA address setup uses the frame's `real_start`. |
| `render.cpp:530–548` | 523 | `aaaf93c9be141dbb7d44e040b072c1516cfc324f73275de2750fd2ff9f0364bd` | Reached renderer size/aspect setup matters to output geometry. |
| `render.cpp:838–912` | 2,747 | `3d24b5d4f1b0de6c51d5a05fb1fcb74fccc7ee55cca7870bc77c1e16f15f8888` | Selected scaler/aspect path can vary output rows; uniform 2× is not assumed. |
| `sdlmain.cpp:3187–3270` | 2,256 | `c705ac880a394a0e917bfd3d4d60224fe4f6aff2e946b13168d936f7b3e1d50e` | Graphics end can return early or take a counter-only OpenGL path before selected backend presentation. |
| `output_surface.cpp:643–778` | 5,755 | `819f1a2838c72ef82e3dab7880c394bbf47b7b143b9ad35d2611aed3df6bb91f` | Surface update calls may discard result; no-change and zero-rectangle paths cannot prove output. |
| `output_opengl.cpp:949–1135` | 7,644 | `ebdba09ceeca5eec13e9b55b7fbf833ec67367abae177313628299e06e968449` | Texture upload and reached swap are distinct; a void swap return does not prove visible completion. |
| `tools/replay/u19_row14_backend_outcome_source.py:35–125` | 3,788 | `24978df8bc96456487de203328197e531b5c31ffc6e6da5e7ae5aa70091cd434` | Pure synthetic `completion_verified` checks supplied witness structure and identity; it cannot authenticate source origin. |
| `tools/replay/cold_start_codeword_row14_completed_backend_rgb24_v2.py:72–154` | 4,455 | `a670b3a9c0e93fda3e1524e82f64f2cf91258bff8b0c3e13f8ffa3a1f5af0e3a` | v2 validates supplied RGB24 bytes, layout and digest; selected labels, backend ID and completion kind remain declarations. |

The public classifier and RGB parser blobs are
`9c02ad3b8b76a38aa0fa892dbef7f49f9191410f` and
`4c4d24c2b83da78f3509871f79d423a719d72da8`. The merged pR page
contract blob is `216dfd690be7322c4a043377bca20dd9e2a2d77c`; its source
pins also cover decoder, palette and runtime format candidates. None of
these inputs supplies a live common issuer or same-frame RGB producer.

## Proposed separately versioned acceptance input

Begin with an accepted selected-write reference and exact eight-label
binding from the separate consumer gate. The later frame/backend/RGB sidecar
must carry one `run_token`, named `clock_origin` and `clock_domain`, unique
strictly increasing issuer indices, and **distinct** write_ref,
plane_event_id, frame_ref, backend_event_id and RGB observation identity.
Equal token text, CPU ordinal, frame counter, packet sequence or host
nanoseconds cannot substitute for a shared issuer chain.

The `frame_ref` is provisional at latch entry. Accept it as a new frame only
after a sourced renderer start with the **actual** `real_start` used by that
frame, plus a sourced page, mode/decoder and palette identity. A guest page
request and queued `display_start` remain separate from the latched page;
the old-frame completion before that assignment retains the old frame_ref.
The future sidecar must source the reached output geometry, clip, scaler,
runtime backend format/pitch/channel conversion and actual canonical RGB24
bytes for this same frame. The current v2 manifest's byte length and digest
can check supplied bytes but cannot source that conversion or frame.

A backend attempt must match this accepted frame_ref, selected backend,
runtime branch and presentation site. The current synthetic classifier uses
an integer `frame_id` and checks a supplied `CompletionWitness`; any future
mapping between that integer and a new frame_ref must itself be sourced and
one-to-one. Its literal `source_domain="independent_backend_completion"`
and `verified=true` are not evidence that a collector observed completion.
The consumer can accept `completion_verified` only after a separately
reviewed independent outcome criterion and matching run, frame_ref,
backend_event_id, backend and site are authenticated. A reached call,
ignored success result, void swap, or texture upload remains at most
`attempt_unverified`. `no_output` must not mint a backend event ID.

The conservative selected-write-to-latch relation without an interior store
hook is `apply_call_exit < latch_enter` under the same issuer. A sourced
interior `actual_store` index could support a narrower relation after its
own review. If the latch enters during the opaque call interval, exact
store order is unresolved. For an accepted new-frame RGB observation,
require sourced new start, renderer output, independently verified backend
completion and RGB observation in strict issuer order for the same frame_ref,
page, `real_start`, palette and conversion identity. The RGB bytes must
also pass the existing v2 parser's exact layout and digest checks.

## Fail-closed negatives and independent gate

Reject the old-frame completion at the latch, missing new start, queued
`display_start` without reached `real_start`, wrong page/decoder/palette,
unproven scaler or channel conversion, renderer abort or no output,
graphics early return, counter-only OpenGL path, surface no-change or zero
rectangles, texture-only update, `no_output` or `attempt_unverified`, a
synthetic verified flag without independent witness provenance, mismatched
backend branch/site/event ID, mixed issuer origin/domain/run, duplicated or
reordered indices, and RGB bytes from a different frame or palette. Equal
page words, eight selected labels or RGB SHA-256 alone cannot repair these
gaps. No current source-only input can pass a **native** completed-frame
acceptance gate.

pR owns frame/page/palette and RGB producer provenance; p9 owns selected
backend outcome and common issuer source; pC owns this future consumer.
First independent gate is a joint pR/p9 source review of frame_ref lifetime,
actual `real_start`, outcome criterion and RGB observation identity against
the exact windows above. Any wire/API or runtime test needs separate review.
This document makes no selected native frame, page, RGB parity or C parity
claim.
