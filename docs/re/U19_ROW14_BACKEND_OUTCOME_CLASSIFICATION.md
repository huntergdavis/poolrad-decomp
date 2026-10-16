# U19 row-14 backend presentation outcome classification

This source-only gate classifies **supplied synthetic** observations of a
selected new frame. It adds no DOSBox-X hook, RGB readback, packet producer,
or native completion proof. Bounded `deja` searches for the U19 backend hook
and common clock returned no indexed finding. Reused checked-in leads are the
[backend completion boundary](U19_ROW14_BACKEND_COMPLETION_SOURCE_BOUNDARY.md),
[backend format variants](U19_ROW14_BACKEND_COMPILE_FORMAT_VARIANTS.md), and
[supplied completed RGB24 packet](U19_ROW14_COMPLETED_BACKEND_FRAME_PACKET_V2.md).

## Pinned source and outcome sites

The ignored DOSBox-X revision is
`5a9c7c53481c2e933948039c33256776653f4faf`. Complete inclusive line
windows retain original newlines:

| Source window | Bytes | SHA-256 | Conditional fact |
| --- | ---: | --- | --- |
| `vga_draw.cpp:4089–4148` | 1,948 | `86621d8c607b3c6440eb2f044445af83ae5a3ee88d07e030450620131662e9ce` | On-demand completion before the new `real_start` assignment may finish the old frame. |
| `render.cpp:472–525` | 2,154 | `c5285bcffae637e87351d8b840652c4ecd968e11846eb9c07d1515358baf59b3` | Renderer return, abort, disabled output, or no `outWrite` can avoid a selected new-frame presentation. |
| `sdlmain.cpp:3187–3270` | 2,256 | `c705ac880a394a0e917bfd3d4d60224fe4f6aff2e946b13168d936f7b3e1d50e` | Graphics early returns precede backend selection; OpenGL may increment a frame counter without output at 3230–3231. |
| `output_surface.cpp:643–778` | 5,755 | `819f1a2838c72ef82e3dab7880c394bbf47b7b143b9ad35d2611aed3df6bb91f` | Surface branches may call SDL2 window updates or SDL1 flips/rect updates, but discard any return status. No-change and zero-rect paths avoid presentation. |
| `output_opengl.cpp:949–1135` | 7,644 | `ebdba09ceeca5eec13e9b55b7fbf833ec67367abae177313628299e06e968449` | xBRZ and normal swaps are at 1026 and 1132; no-change returns precede normal swap. Texture upload is not a swap. |
| `sdlmain.cpp:10223–10225` | 69 | `b54768a51e3e65c951ad52e418647e8558a09450872da9c9c64c34baf46b8311` | The SDL2 OpenGL wrapper is `void` and calls `SDL_GL_SwapWindow`. |
| `vs/sdl/include/SDL_video.h:858–867` | 298 | `e737aa6224d15cbce801d7f4d19c85f921243f740a713b23a140824ac61166eb` | The vendored SDL1 `SDL_GL_SwapBuffers` declaration is also `void`. |
| `vs/sdl/include/SDL_video.h:390–417` | 1,112 | `cd8d44101de3fc59471acfb2fe804fa7ce98a43fa468d11dffbab28ef48ab24b` | The vendored SDL1 declaration makes `SDL_UpdateRects` void and `SDL_Flip` return `int`. |
| `vs/sdl2/include/SDL_video.h:1368–1404` | 1,426 | `9c622091ac3de8f41d7b2e8fe04a55e4bd3a568d099f0a178892c3e1a1ad07df` | The vendored SDL2 window-surface update functions return `int`. |

The full DOSBox-X source-file hashes are in the merged backend completion
and format notes. The branch's compile mode and selected runtime backend for
a saved U19 run remain unmeasured. The distinct source call sites are:

| Branch | Presentation call lines | Outcome available in current code |
| --- | --- | --- |
| SDL2 surface | `687`, `716`, `737`, `744`, `771` in `output_surface.cpp` | `int` results are discarded; line `771` requires a positive rectangle count. |
| SDL1 surface | `689`, `718`, `739`, `746`, `773` in `output_surface.cpp` | Flip results are discarded; line `773` is a void rectangle update requiring positive count. |
| OpenGL | `1026`, `1132` in `output_opengl.cpp` | Swap wrapper has no success value; the counter-only and no-change paths do not reach these calls. |

## Pure synthetic classifier boundary

`tools/replay/u19_row14_backend_outcome_source.py` accepts one typed
`Attempt` with supplied run, frame, selected backend, source call site,
new-frame/abort/call-reach state, rectangle count when applicable, and an
optional externally observed API result. That result is **not** captured by
the original code. A `backend_event_id` is required only for a reached
selected-frame outcome; the `no_output` path must not mint one.

| Result | Required supplied facts |
| --- | --- |
| `no_output` | The selected new frame was not started, renderer aborted, the presentation call was not reached, or a rectangle path had zero rectangles. A completion witness or new backend event ID here is rejected. |
| `attempt_unverified` | A selected-frame presentation call was reached, but no independently sourced matching completion witness exists. A returned zero from an `int` API alone remains an attempt; a known failure remains an attempt and conflicts with a verified witness. |
| `completion_verified` | A separately supplied `CompletionWitness` declares independent verification and exactly matches run, frame, backend event ID, backend, and call site, with no contradictory API failure. This classifier checks only that supplied structure; it cannot authenticate the witness's origin. |

The synthetic witness's `source_domain` literal is a declared contract, not
proof that an independent collector ran. A real producer would need a
separately reviewed same-frame completion criterion, common host event clock,
runtime backend/format/pitch/clip/scaler identity, and canonical 640×400
RGB24 conversion. A successful API return by itself is narrower than
verified display completion, and the SDL1 rectangle and OpenGL swap calls
have no result to check in this source path. No output from this classifier
can be passed off as native `completion_kind=backend_complete` evidence.

The focused media-free test uses invented tokens and five cases covering all
12 call sites, ignored success returns, counter-only/no-change/zero-rectangle
and old-frame/abort stops, same-frame synthetic verification, mismatched
identity, void-result rejection, and a contradictory failed API result:

```sh
python3 -m unittest -v tests.replay.test_u19_row14_backend_outcome_source
```

pR cleared this distinct documentation/classifier/test scope in durable inbox
`i10117`; PM authorized local work in `i10130`. p5 owns the handler receipt,
pC owns the versioned consumer, and pR owns the completed RGB24 parser.
None of those files is edited. Original game media, private emulator source,
and any future RGB buffers remain ignored under `resources/`. This gate
establishes no selected native backend, completed host frame, displayed page,
RGB parity, or C parity.
