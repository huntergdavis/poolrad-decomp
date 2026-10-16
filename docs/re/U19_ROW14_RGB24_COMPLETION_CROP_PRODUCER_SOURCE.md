# U19 row-14 completed RGB24 frame and crop producer boundary

This read-only source map separates a completed host RGB24 frame from the
selected U19 CPU-store sidecar. It neither captures a native frame nor binds a
frame to a guest store. A bounded `deja` query for the U19 completed RGB24
producer, page, crop, and selected event found no indexed session. Reused
evidence is the merged [presentation boundary](U19_ROW14_PRESENTATION_EVENT_BOUNDARY.md),
[host source chain](U19_ROW14_HOST_RGB_SOURCE_CHAIN.md),
[expected crop](U19_ROW14_EXPECTED_RGB_CROP.md), and
[same-run join contract](U19_ROW14_SAME_RUN_JOIN_CONTRACT.md).

## Pinned conditional host path

The read-only DOSBox-X source audit passes at private revision
`5a9c7c53481c2e933948039c33256776653f4faf`, with heavy executable
SHA-256 `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.
It checks these complete line windows without recording image content:

| Host source window | SHA-256 | Required distinction |
| --- | --- | --- |
| `vga_draw.cpp:4089–4148` | `86621d8c607b3c6440eb2f044445af83ae5a3ee88d07e030450620131662e9ce` | A latch callback can complete the old frame before assigning `real_start=display_start`. |
| `vga_draw.cpp:6190–6220` | `7f576f607defb5f76e47c1a62b3cf5d6a4d6adf56681f0e21a08f49150bdf74b` | A timer can end a prior renderer update before starting the next frame. |
| `render.cpp:472–525` | `c5285bcffae637e87351d8b840652c4ecd968e11846eb9c07d1515358baf59b3` | End-update can return without an active frame or without output. |
| `sdlmain.cpp:3187–3270` | `c705ac880a394a0e917bfd3d4d60224fe4f6aff2e946b13168d936f7b3e1d50e` | Backend selection and successful completion must be observed, not inferred from entry. |

The conditional pixel path runs through EGA decoder selection
`vga_draw.cpp:7740–7762`, one selected planar decoder, line draw
`3956–4024`, the render cache/output gate `render.cpp:320–440`, and the
completed backend. This source path does not identify the saved run's decoder,
selected page, emitted frame, or actual RGB24 producer.

## Required typed handoff

The p5 version-2 selected-event adapter checks eight CPU-source identity
fields: `run_token`, `arm_token`, `stream_id`, `plane_event_id`, `raw_sha256`,
`candidate_ordinal`, `pc`, and `successor_pc`. It preserves its separately
audited source/raw digests and false `native_coverage_proved` flag. Those
fields do not contain a host frame, latch phase, backend completion, RGB24
bytes, or common event clock. The merged version-1 `HostWitness` shape names
`frame`, `real_start`, `backend`, `backend_event_id`, `rgb_sha256`, palette
identity, and 640×400 dimensions, but the current join caps store ordinals at
128; the fixed 36-cell route can first visit `A000:118F` at ordinal 897 or
898. A version-1 object cannot be relabeled as a version-2 observation.

A future host producer must separately validate one run-wide order:
`crtc_sample < latch_enter < latch_assign < renderer_start <
renderer_end(output=True) < backend_complete < rgb_capture`. The selected
store must precede renderer end. The backend and RGB capture must share a
frame and `backend_event_id`; frame start must retain the **latched**
`real_start` and palette identity it used. An end-update entry, a prior-frame
completion inside the latch callback, or a digest without the canonical
RGB24 bytes fails this handoff. Guest DS:4A16/4A18 page words, programmed
`display_start`, latched `real_start`, and live DRAW address remain distinct.
Guest frame indices and host event indices are separate clocks.

The existing typed `RGB24Observation` accepts supplied bytes with
`row_stride >= width*3` and exact `row_stride*height` length. The supplied
frame must be 640×400 with verified RGB24 channel order and palette; a
640×400 digest alone cannot feed the comparator. The U19 expected crop is
`x=16,y=224,width=576,height=16`, tightly packed stride 1,728, length
27,648, SHA-256
`e6f630a6abf12a030e92387a2966f7ab15abff63f8a8a57b9b8988e516dff980`.
For each crop row `r`, the source interval in a supplied host frame begins
at `(224+r)*row_stride + 16*3` and spans `576*3` bytes. The existing
`compare_patch` uses that stride and ignores any host row padding. The
retained `+0302` complete-frame hash
`b01aed35573d243847675d969712a3606336ca28dd9e2d34885e965c8040a683`
is an endpoint only; it has no selected-store or page clock.

Current status: **BLOCKED_HOST_RGB_PRODUCER_AND_V2_JOIN**. The p5 adapter
authenticates a supplied version-1 CPU stream only to a source-only selected
identity; the current branch has no validated same-run completed-frame
producer or version-2 host/plane join. No live RGB attribution, first plane
change, page transition, C parity, or full-frame count follows.

Read-only reproduction uses the pinned ignored private inputs in the local
checkout, without launching DOS:

```sh
python3 tools/replay/cold_start_codeword_row14_presentation_path_audit.py \
  --source resources/dosbox-x-16a1-private-src \
  --heavy resources/dosbox-x-heavy-src/src/dosbox-x
```

Original media, host frames, palette, plane bytes, and any emulator trace
remain ignored under `resources/`; this note publishes addresses, hashes,
geometry, and source-only conditions.
