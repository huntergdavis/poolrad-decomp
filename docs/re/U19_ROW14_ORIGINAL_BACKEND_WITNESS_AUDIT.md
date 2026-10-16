# U19 row-14 original backend completion witness audit

This bounded source audit asks whether the original selected presentation
path produces a verified completion witness. It does not: the reached
surface calls discard available results, the OpenGL swap wrappers have no
result, and the current U19 tools validate supplied outcome fields without
authenticating a backend event. This is a source-path conclusion, not a
saved-run backend selection or a claim that an external display API can
never complete. Two bounded `deja` searches for the U19 backend witness
and common clock returned no indexed finding. This audit rechecks the
merged [backend outcome classifier]
(U19_ROW14_BACKEND_OUTCOME_CLASSIFICATION.md), [completion boundary]
(U19_ROW14_BACKEND_COMPLETION_SOURCE_BOUNDARY.md), and [supplied RGB24 v2
packet](U19_ROW14_COMPLETED_BACKEND_FRAME_PACKET_V2.md).

## Exact source lock

The original DOSBox-X source is ignored revision
`5a9c7c53481c2e933948039c33256776653f4faf`. The two public U19 tools
below are from main `ef587349de5405d959b184661534f397dc42ce99`;
their complete SHA-256 values are
`eefa68f5ae4e03daa17f52ee8553554a4db42f759d90fba41736cb42f63fe22a`
(`u19_row14_backend_outcome_source.py`) and
`9ddbee8228bc5f8009408f603631e5fc2471e75c1c32e98af5756deeade5c426`
(`cold_start_codeword_row14_completed_backend_rgb24_v2.py`). Complete
inclusive windows retain original newlines.

| Source window | Bytes | SHA-256 | Bounded fact |
| --- | ---: | --- | --- |
| `vga_draw.cpp:4089–4148` | 1,948 | `86621d8c607b3c6440eb2f044445af83ae5a3ee88d07e030450620131662e9ce` | The latch may complete the old frame before new `real_start`. |
| `render.cpp:472–525` | 2,154 | `c5285bcffae637e87351d8b840652c4ecd968e11846eb9c07d1515358baf59b3` | Renderer end can abort, skip output, or return before selected backend dispatch. |
| `sdlmain.cpp:3187–3270` | 2,256 | `c705ac880a394a0e917bfd3d4d60224fe4f6aff2e946b13168d936f7b3e1d50e` | `GFX_EndUpdate` chooses a backend only after early returns; OpenGL has a counter-only return. |
| `output_surface.cpp:643–778` | 5,755 | `819f1a2838c72ef82e3dab7880c394bbf47b7b143b9ad35d2611aed3df6bb91f` | Surface branches call SDL update/flip functions without checking results; no-change and zero-rectangle paths can avoid the call. |
| `output_opengl.cpp:949–1135` | 7,644 | `ebdba09ceeca5eec13e9b55b7fbf833ec67367abae177313628299e06e968449` | xBRZ and normal paths can swap at 1026 and 1132; texture upload is earlier, and no-change paths can return first. |
| `sdlmain.cpp:10223–10225` | 69 | `b54768a51e3e65c951ad52e418647e8558a09450872da9c9c64c34baf46b8311` | The SDL2 OpenGL swap wrapper is `void`. |
| `vs/sdl/include/SDL_video.h:390–417` | 1,112 | `cd8d44101de3fc59471acfb2fe804fa7ce98a43fa468d11dffbab28ef48ab24b` | SDL1 `SDL_Flip` returns `int`, while rectangle updates are `void`. |
| `vs/sdl/include/SDL_video.h:858–867` | 298 | `e737aa6224d15cbce801d7f4d19c85f921243f740a713b23a140824ac61166eb` | SDL1 OpenGL swap is declared `void`. |
| `vs/sdl2/include/SDL_video.h:1368–1404` | 1,426 | `9c622091ac3de8f41d7b2e8fe04a55e4bd3a568d099f0a178892c3e1a1ad07df` | SDL2 window-surface update functions return `int`. |
| `u19_row14_backend_outcome_source.py:35–63` | 585 | `b89983fcec9d38be482c0be3668ee7ca16bcdc7fd878d1f7f62064d308bb65bd` | `Attempt` and `CompletionWitness` are supplied values, not original backend receipts. |
| `u19_row14_backend_outcome_source.py:100–137` | 1,795 | `027fbe11f2212b7cbfa69b3df427330f1d889c66ac53a853bdf8c3ba517cff3e` | Structural witness matching can classify `completion_verified`, but cannot authenticate witness origin. |
| `cold_start_codeword_row14_completed_backend_rgb24_v2.py:20–65` | 1,116 | `91c2d03cf39f6d8b14d11deda93ba8951e0e15fe8707a4a6163b5b2a069da793` | The manifest declares `backend_event_id`, `completion_kind`, and canonical RGB24 layout. |
| `cold_start_codeword_row14_completed_backend_rgb24_v2.py:120–166` | 2,397 | `490cc312c71b9a2f246ad1ae9e5d44565ba0c7a1da4a88b152937d28183014ba` | The parser checks supplied values and bytes; it does not obtain a backend outcome, frame, page, or host clock. |

## Original call versus verified witness

| Selected branch | Original presentation calls | What the unmodified path records |
| --- | --- | --- |
| SDL2 surface | `output_surface.cpp` 687, 716, 737, 744, 771 | The `int` result is discarded at every call; reaching a call is an attempt. The rectangle call at 771 requires a positive count. |
| SDL1 surface | `output_surface.cpp` 689, 718, 739, 746, 773 | Flip's `int` result is discarded. Rectangle update at 773 is `void` and requires a positive count. |
| OpenGL | `output_opengl.cpp` 1026, 1132 | The swap returns no result to this code. The wrapper and prior texture upload do not produce a verified same-frame receipt. |

The original call chain does not mint a `backend_event_id`, bind a backend
outcome to the handler's selected eight-label receipt, or read a canonical
640×400 RGB24 frame after verified presentation. A successful SDL return,
if separately observed later, would still need a reviewed meaning and
same-frame binding; an ignored return, a void swap, `frames++`, and
`actual_frame_count++` cannot themselves authenticate completion. An old
frame finished inside the latch, a render abort, an early graphics return,
a no-change/zero-rectangle surface branch, or OpenGL's counter-only branch
cannot be labeled as selected new-frame completion.

The merged classifier's `completion_verified` is conditional on a caller
supplying a matching `CompletionWitness` with a declared independent source
domain. It validates structure; it cannot check that a collector exists or
that a display operation completed. The v2 parser's `completion_kind` and
`backend_event_id` are likewise manifest declarations. Neither tool
provides the original backend receipt or converts a supplied string into
native proof.

A future producer must first identify the active compile and runtime
backend, the selected frame's `real_start`, run/frame identity and common
host issuer, and the exact call site. It must distinguish no-output from an
attempt and independently verify completion for that same frame; canonical
RGB24 conversion and its page/palette/scaler identity need separate source
review. This audit does not select a runtime backend, define that
verification criterion, or implement a new packet.

p5, pC, and pR cleared this distinct source-only note in durable inbox
`i10317`, `i10316`, and `i10319` as relayed by PM in `i10334`. Their
handler, consumer, parser, and wire files are untouched. Original source,
game media, traces, and RGB buffers remain ignored under `resources/`.
There is no native backend completion, displayed page, RGB parity,
full-frame, or C parity claim.
