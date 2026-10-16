# U19 row-14 selected-backend completion source boundary

This source-only audit identifies where a future host receipt could distinguish a
selected backend presentation attempt from a skipped or old-frame update. It
does not install a hook, observe a saved run, establish that an API call
succeeded, or validate an RGB frame. A bounded `deja "Poolrad U19 backend
completion source"` query produced no usable result within 12 seconds. The
reused leads are the merged [presentation boundary](U19_ROW14_PRESENTATION_EVENT_BOUNDARY.md),
[completed-frame producer boundary](U19_ROW14_RGB24_COMPLETION_CROP_PRODUCER_SOURCE.md),
and [supplied v2 packet](U19_ROW14_COMPLETED_BACKEND_FRAME_PACKET_V2.md).

## Original-source lock

The ignored DOSBox-X source checkout is revision
`5a9c7c53481c2e933948039c33256776653f4faf`. Full-file SHA-256 values
are `f9b798311cffb633754145cfc95b559f180a01b7e6f9f111ef47b7f56e79518d`
(`vga_draw.cpp`), `2c0d47c3c7d73fe092009a7d883316a6e40cf5620723f952c8278e7d93bac1d6`
(`render.cpp`), `312bf64551af8d939bf1aeefa994d3273030f9caa97017db8be7b379e5689f02`
(`sdlmain.cpp`), `60670d41fe31cb6ecf65343bd540beaae93d9e0c39f27495fd63a70c1cd4067e`
(`output_surface.cpp`), and `6b4f203681fd19b7c9b6b17a99d94ad5a95e1cddade9372d49a93d0c02fd4a54`
(`output_opengl.cpp`). Line windows are inclusive and retain original newlines.

| Source window | Bytes | SHA-256 | Conditional boundary |
| --- | ---: | --- | --- |
| `vga_draw.cpp:4089–4148` | 1,948 | `86621d8c607b3c6440eb2f044445af83ae5a3ee88d07e030450620131662e9ce` | A latch callback may finish the **old** frame before assigning the new `real_start`. |
| `vga_draw.cpp:6190–6220` | 825 | `7f576f607defb5f76e47c1a62b3cf5d6a4d6adf56681f0e21a08f49150bdf74b` | The vertical timer can end an update and return before starting a new frame. |
| `render.cpp:472–525` | 2,154 | `c5285bcffae637e87351d8b840652c4ecd968e11846eb9c07d1515358baf59b3` | `RENDER_EndUpdate` can return, skip graphics output, or call `GFX_EndUpdate`; image capture occurs before that call. |
| `sdlmain.cpp:3187–3270` | 2,256 | `c705ac880a394a0e917bfd3d4d60224fe4f6aff2e946b13168d936f7b3e1d50e` | `GFX_EndUpdate` can return before backend dispatch, including the OpenGL frame-counter-only path; its switch selects the backend. |
| `output_surface.cpp:643–778` | 5,755 | `819f1a2838c72ef82e3dab7880c394bbf47b7b143b9ad35d2611aed3df6bb91f` | Surface xBRZ, aspect and direct branches issue SDL2 window updates or SDL1 flips/rect updates; no-change and zero-rectangle paths can avoid presentation. |
| `output_opengl.cpp:949–1135` | 7,644 | `ebdba09ceeca5eec13e9b55b7fbf833ec67367abae177313628299e06e968449` | OpenGL xBRZ and normal paths can reach swaps at lines 1026 or 1132; no-change returns precede the normal swap. Texture upload alone is not a swap. |
| `sdlmain.cpp:10223–10225` | 69 | `b54768a51e3e65c951ad52e418647e8558a09450872da9c9c64c34baf46b8311` | The SDL2 OpenGL wrapper delegates to `SDL_GL_SwapWindow`. |

The surface code does not inspect the result of its update/flip calls. The
OpenGL wrapper has no success value to pass back. Therefore source reachability
to a call, and even a return from this unmodified code, cannot by itself prove
a successful displayed frame. A future completion hook must select the active
backend branch, distinguish a presentation call from every early return,
record the available API outcome or an independently verified completion
criterion, and bind that result to the frame that actually entered the branch.

## Bounded future receipt

A producer-side receipt would need a run token, frame identity, the frame-start
`real_start` and latch phase, selected backend and compile branch, render/output
flags, changed-line classification, exact presentation call site, and an
explicit outcome class such as `no_output`, `attempt_unverified`, or
`completion_verified`. The receipt must be emitted only after the selected
branch's outcome is known. A separately sourced backend event identity can
then name that receipt; its common clock origin/domain and relation to handler
phases require a separate source audit. This is a requirement sketch, not a
new packet or implemented API.

Reject as evidence of a new-page completion: a preassignment old-frame update
inside the latch callback; an inactive or aborted renderer without separate
backend proof; an image capture before graphics output; an early
`GFX_EndUpdate` return; the OpenGL
frame-counter-only return; a surface no-change/zero-rectangle branch; a
texture upload without a swap; or a call whose outcome is not checked. The
merged v2 RGB24 parser accepts supplied `completion_kind` and
`backend_event_id` declarations but cannot authenticate any of these paths.
No selected backend, page, host event, RGB conversion, native frame, or C
parity is established here.

To reproduce, hash each complete line window in the ignored source revision
above. Keep original source copies, game media, captures and RGB buffers under
ignored `resources/`; this note publishes only source locations, hashes,
numeric bounds and conditional relationships.
