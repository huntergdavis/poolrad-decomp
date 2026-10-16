# U19 row-14 backend format to RGB24 source boundary

This read-only screen asks what a future **completed backend** image must
carry before it can be treated as canonical 640×400 RGB24 for the U19 row-14
crop. It does not select a backend, copy pixels, run DOS, or validate a
same-run frame. A bounded `deja` query for the DOSBox-X U19 RGB24 channel
order and `real_start` crop timed out without a usable result. Reused leads
are the merged [completed-frame boundary](U19_ROW14_RGB24_COMPLETION_CROP_PRODUCER_SOURCE.md),
[presentation source](U19_ROW14_PRESENTATION_EVENT_BOUNDARY.md), and
[typed visual witness](U19_VISUAL_WITNESS_QA_CONTRACT.md).

## Private source lock

The read-only DOSBox-X checkout is revision
`5a9c7c53481c2e933948039c33256776653f4faf`. These exact line windows
were hashed from the ignored source; the full files hash to
`60670d41fe31cb6ecf65343bd540beaae93d9e0c39f27495fd63a70c1cd4067e`
(`output_surface.cpp`),
`312bf64551af8d939bf1aeefa994d3273030f9caa97017db8be7b379e5689f02`
(`sdlmain.cpp`), and
`6b4f203681fd19b7c9b6b17a99d94ad5a95e1cddade9372d49a93d0c02fd4a54`
(`output_opengl.cpp`).

| Source window | SHA-256 | Conditional fact |
| --- | --- | --- |
| `output_surface.cpp:178–209` | `d889550125663aa9308f1f98ec1295b34759517415304495e5e1fc4c679d6bde` | SDL2 gets the window surface at runtime and accepts 8/15/16/32-bit formats; the source does not fix its channel layout. |
| `output_surface.cpp:599–642` | `9c98355651e5503d4c1a55e0c226d6039ad27f02b3ab3395678ae8ba51f88b04` | Start-update can write an xBRZ/aspect buffer, blit surface, or clipped window surface, each with a distinct pointer and pitch. |
| `output_surface.cpp:643–778` | `819f1a2838c72ef82e3dab7880c394bbf47b7b143b9ad35d2611aed3df6bb91f` | End-update can scale/blit, return for unchanged lines, or update only rectangles; entry alone is not completed presentation. |
| `sdlmain.cpp:3103–3122` | `de4dd1ff69c00d65ee15f11918c0e7995d466d1f21d4e9539ea86a0fed82ae8a` | `GFX_GetSurfacePtr` adds `y*pitch + x*(BitsPerPixel >> 3)` to the surface pointer. That expression is not the start-update clip expression. |
| `sdlmain.cpp:3133–3159`, `3187–3270` | `f31a1fbb5dd842612b0f8bd457cbcad65473bb1a844356e7974dea4a5cbcbccc`, `c705ac880a394a0e917bfd3d4d60224fe4f6aff2e946b13168d936f7b3e1d50e` | Start/end dispatch on `sdl.desktop.type`; early returns and backend selection remain possible. |
| `sdlmain.cpp:3300–3328` | `0c78e3c9fde8f17a5c8c1054e52e8ff229d97d1d3e16f2c04a030ac0ab5c8b2d` | Surface colors use `SDL_MapRGB(surface->format, r,g,b)`; OpenGL packs a different word and has a platform branch. A packed word is not canonical RGB byte order. |
| `output_opengl.cpp:921–956`, `949–1035` | `b1dc492a228c3ec7c057323e4fbde2610bc5bb941eff14fa6cdcb64beda5d669`, `15f81f39d22d576eb8e7b28b1d1c522ba07417a9fd3569e7863e46f1d1485786` | OpenGL may write a PBO or frame buffer, then take a separate upload/presentation path. Surface pitch/masks cannot be reused for it. |

The start-update direct-surface branch instead adds
`clip.y*pitch + clip.x*format->BytesPerPixel`. The source accepts a 15-bit
surface, for which `15 >> 3` is 1; the two expressions cannot be assumed
equivalent or to select the correct storage width without the selected
runtime path and SDL format. This is a pointer/conversion boundary, not a
measured bad image.

The merged `GFX_EndUpdate` source lock already distinguishes a renderer
end call from successful selected-backend completion. This note adds the
**pixel-format** boundary after that gate. A captured `sdl.surface->pixels`
buffer cannot be interpreted as RGB24 by length alone: its width, height,
pitch, `BitsPerPixel`, `BytesPerPixel`, R/G/B masks and shifts, clip origin,
selected backend, scaling/aspect path, and completed update must belong to
the same frame. A 32-bit surface word must be decoded according to its
runtime format; a packed OpenGL upload word follows another path. A
guest-side VGA screenshot encoder is a separate capture path and does not
authenticate a completed display backend frame.

The existing `RGB24Observation` layout check accepts positive dimensions,
`row_stride >= width*3`, and exact `row_stride*height` length. The separate
U19 same-run host contract requires 640×400. Neither checker proves that
the supplied bytes were converted from the
selected backend or that channel order is RGB. A future producer must
declare and verify that conversion, plus the frame/`backend_event_id`,
latched page, palette identity and event order from the merged
[same-run join contract](U19_ROW14_SAME_RUN_JOIN_CONTRACT.md). Only then can
the page-neutral expected patch at `(16,224,576,16)` be compared using the
typed stride. The source-derived 27,648-byte patch SHA-256 is
`e6f630a6abf12a030e92387a2966f7ab15abff63f8a8a57b9b8988e516dff980`;
it is not a digest of a newly observed host frame.

Current status: **BLOCKED_RUNTIME_BACKEND_FORMAT_AND_RGB24_PRODUCER**. No
native backend, RGB frame, first plane change, selected page, pixel parity,
or full-frame credit follows. To reproduce the source lock, use the private
revision above and SHA-256 over each complete line window (inclusive,
preserving original newlines); no emulator execution is required. Original
source copies, screenshots, raw buffers, palette and game media stay ignored
under `resources/`. This public note contains only source locations,
hashes, numeric geometry, and conditional relationships.
