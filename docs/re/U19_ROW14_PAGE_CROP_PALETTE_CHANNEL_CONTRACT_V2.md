# U19 row-14 page, crop, palette, and channel contract v2

This source-only contract names the evidence needed to interpret a **supplied**
640×400 RGB24 frame as the saved U19 row-14 display. It adds no collector,
packet field, parser, pixel comparison, or native observation. The merged
[RGB24 v2 packet](U19_ROW14_COMPLETED_BACKEND_FRAME_PACKET_V2.md) authenticates
the supplied byte extent and digest only. Its selected CPU labels and backend
completion remain declarations. pC owns their future association consumer;
p9 owns the original backend-format and completion-hook source pins.

The host-heavy hold precludes another `deja` lookup. This note reuses the
bounded, unsuccessful recall attempts and exact source locks already reported
in the [page-to-host bridge](U19_ROW14_PAGE_TO_HOST_SOURCE_BRIDGE.md),
[scanline/crop source map](U19_ROW14_SCANLINE_CROP_SOURCE.md),
[same-frame field contract](U19_ROW14_SAME_FRAME_SCANLINE_SCALER_CAPTURE.md),
and [backend-format boundary](U19_ROW14_BACKEND_RGB24_FORMAT_SOURCE.md).
Those notes pin ignored DOSBox-X revision
`5a9c7c53481c2e933948039c33256776653f4faf`; this note does not claim a
new private-source audit. The fixed expected patch comes from the separate
[36-cell C producer](U19_ROW14_EXPECTED_RGB_CROP.md), whose original
`GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.

## Conditional source pins reused

| Boundary | Pinned source window and SHA-256 | Consequence |
| --- | --- | --- |
| Guest page request | `int10_char.cpp:1184–1213` `17d105217bf5a6d88f4416a3d38b8f22ac61c8d0bcf564fb4874040c0ad1906e` | A reached INT 10h AH=05 can program CRTC start; a guest page word alone cannot. |
| Latched frame page | `vga_draw.cpp:4119–4148` `bffc6f2bee77aff833ea09a015384a2700ae3f5b266a3673de1f761e773c4000`; `:6259–6290` `888d758087aa016fa34a920fd9913b6393723e920c22478c128ad5f3e3e9b9a9` | `display_start` and the frame's `real_start` are distinct; EGA address setup uses the latter. |
| Source line and decoder | `vga_draw.cpp:3956–4038` `c3ce297f472f8fd67f0b4b41ce8e9f0927cdb2c5fa8052ee87502487c287c963`; `:7738–7778` `c135178ee4d3d5d69b455e8e0f69dc4587e3ce824d1c5b461b1c05e656cb6bc2` | Skip/blank/decode branches and the selected EGA expander decide which plane address yields a line. |
| Host geometry | `render.cpp:530–548` `aaaf93c9be141dbb7d44e040b072c1516cfc324f73275de2750fd2ff9f0364bd`; `:838–912` `3d24b5d4f1b0de6c51d5a05fb1fcb74fccc7ee55cca7870bc77c1e16f15f8888` | Aspect and scaler paths can assign nonuniform output rows; a configured 2× mode does not establish the reached mapping. |
| Output format | `output_surface.cpp:178–209` `d889550125663aa9308f1f98ec1295b34759517415304495e5e1fc4c679d6bde`; `sdlmain.cpp:3300–3328` `0c78e3c9fde8f17a5c8c1054e52e8ff229d97d1d3e16f2c04a030ac0ab5c8b2d` | Surface depth, pitch, masks and `SDL_MapRGB` depend on the selected runtime format; a packed word is not RGB byte order. |

The [backend-format boundary](U19_ROW14_BACKEND_RGB24_FORMAT_SOURCE.md)
separately pins SDL surface clipping, start/end update and OpenGL PBO/upload
windows. The [scanline contract](U19_ROW14_SAME_FRAME_SCANLINE_SCALER_CAPTURE.md)
pins split, panning, vblank, line counters, decoder input and output-row
events. These source windows identify candidate observation sites; they do
not say which branch ran in the saved U19 interval.

## Minimum same-frame receipt

A future producer must retain one frame identity and common host event domain
through all of the following, with each field sourced at the stage that uses
it. An absent field is a typed blocker, not a default inferred from the
expected picture.

1. **Page and frame start:** reached guest page-helper call/return if a page
   request is claimed; CRTC 0C/0D, pending `display_start`, the later latch
   assignment, the frame's actual `real_start`, mode/machine and display
   overrides. Keep the old frame's end separate from the new frame's start.
2. **Plane-to-source line:** selected EGA expander, palette translation state,
   initial address, address shift/mask, byte and line skips, scan length,
   split/line-compare, vblank and panning. For each source line covering the
   36-cell row, retain the reached pre/post address and whether that line was
   skipped, blanked, decoded and accepted by `RENDER_DrawLine`.
3. **Source-to-host pixels:** reached `RENDER_SetSize`, selected scaler/aspect
   handler, its consumed counters and aspect entries, clip and output pitch,
   and the actual output x/row intervals for those source pixels. A uniform
   2× result is a measured special case, not a configuration assumption.
4. **Palette-to-channel bytes:** effective AC/DAC palette and its identity at
   decode, selected backend and completed update, runtime surface depth,
   bytes per pixel, pitch and R/G/B masks/shifts, or the distinct OpenGL
   upload format and path. Record the conversion that produced canonical
   RGB24 bytes and its clip/scaling origin for the **same frame**. A supplied
   `pixel_format="RGB24"` label and SHA-256 cannot establish this conversion.

The source-derived ideal row is 36 cells × 8 pixels, source x `8..295` and
y `112..119`. If all selected source and output branches actually produce a
uniform 2× mapping with no offset, split, panning, skip, overlay or palette
change, the expected host rectangle is `(16,224,576,16)`, tightly packed
27,648 bytes, SHA-256
`e6f630a6abf12a030e92387a2966f7ab15abff63f8a8a57b9b8988e516dff980`.
The expected EGA color-10 RGB `(85,255,85)` also depends on that expected
palette. A different reached palette or output mapping changes the proposed
comparison, even when the supplied full-frame RGB24 digest is valid.

## Rejection boundaries

Reject a claimed page/crop provenance when it uses only equal guest page
words, a queued `display_start` without the frame's latch, a mid-frame draw
address in place of `real_start`, an old-frame completion, a skipped EGA line,
an assumed uniform scaler, a palette identity from another frame, or surface
masks applied to an OpenGL buffer. A 15-bit SDL surface and a clipped or
padded surface require their actual selected byte width and origin. The
existing `compare_patch` can count differences in supplied canonical bytes;
it cannot reject these provenance failures on its own.

Status: **BLOCKED_SAME_FRAME_PAGE_CROP_PALETTE_CHANNEL_SOURCE**. p9's source
pins are conditional, p5's handler receipt/phase and pC's v2 association
consumer are separate, and no completed same-run host frame is captured by
this note. No selected page, first writer, live crop, RGB parity, C parity,
or full-frame credit follows. Private source, media, palette, planes and
images remain under ignored `resources/`.
