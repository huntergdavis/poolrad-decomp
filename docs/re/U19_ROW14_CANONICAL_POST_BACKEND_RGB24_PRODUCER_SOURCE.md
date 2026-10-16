# U19 row-14 canonical post-backend RGB24 producer source boundary

This source-only note asks where a **completed** backend frame could become
canonical RGB24 bytes for the saved U19 row-14 comparison. It adds no hook,
readback, parser, packet field, test, or native observation. The existing
[completed-frame producer boundary](U19_ROW14_RGB24_COMPLETION_CROP_PRODUCER_SOURCE.md)
and [page/crop/palette contract](U19_ROW14_PAGE_CROP_PALETTE_CHANNEL_CONTRACT_V2.md)
remain the source of the conditional guest-to-host path. This note narrows
their open backend result and byte-conversion gates. The host-heavy hold
precludes a repeat `deja`; it reuses those notes' bounded recall and source
locks. p9 owns original issuer and backend-witness audits; pC owns the future
consumer. Ownership clearance does not verify a native frame.

The ignored DOSBox-X source revision is
`5a9c7c53481c2e933948039c33256776653f4faf`. The following complete,
inclusive source windows retain original newlines. Their SHA-256 values were
rechecked locally; they identify **candidate** sites, not reached U19 events.

| Source window | Bytes | SHA-256 | Bounded result |
| --- | ---: | --- | --- |
| `render.cpp:472–525` | 2,154 | `c5285bcffae637e87351d8b840652c4ecd968e11846eb9c07d1515358baf59b3` | `CAPTURE_AddImage` uses the renderer cache and palette before `GFX_EndUpdate`; it cannot be the post-backend RGB24 observation. Renderer end can also skip output. |
| `sdlmain.cpp:3187–3270` | 2,256 | `c705ac880a394a0e917bfd3d4d60224fe4f6aff2e946b13168d936f7b3e1d50e` | Graphics end can return before dispatch; the OpenGL counter-only branch returns without backend update. Selected backend and output path must be retained. |
| `output_surface.cpp:643–778` | 5,755 | `819f1a2838c72ef82e3dab7880c394bbf47b7b143b9ad35d2611aed3df6bb91f` | No-change and zero-rectangle paths can skip presentation. SDL surface update/flip results at the selected call sites are discarded. |
| `output_opengl.cpp:949–1135` | 7,644 | `ebdba09ceeca5eec13e9b55b7fbf833ec67367abae177313628299e06e968449` | Texture/PBO upload and swap are separate; no-change can return, and reached swap is not an independently verified visible completion. |
| `output_surface.cpp:178–209` | 1,369 | `d889550125663aa9308f1f98ec1295b34759517415304495e5e1fc4c679d6bde` | SDL2 window-surface format and depth are runtime state. |
| `output_surface.cpp:532–565` | 1,196 | `a4a4b070ea19824b201f2ad73f023b07620b47cc7ff2ac16e85200f9a1f8c2b5` | SDL1 mask/copy handling differs from the SDL2 surface path. |
| `output_surface.cpp:599–642` | 1,535 | `9c98355651e5503d4c1a55e0c226d6039ad27f02b3ab3395678ae8ba51f88b04` | Surface start pointer, pitch and clipping determine byte origin. |
| `sdlmain.cpp:3103–3122` | 616 | `de4dd1ff69c00d65ee15f11918c0e7995d466d1f21d4e9539ea86a0fed82ae8a` | Surface pointer stepping uses `BitsPerPixel >> 3`; a 15-bit path needs its reached width checked rather than an assumed three-byte pixel. |
| `sdlmain.cpp:3300–3328` | 1,072 | `0c78e3c9fde8f17a5c8c1054e52e8ff229d97d1d3e16f2c04a030ac0ab5c8b2d` | `SDL_MapRGB` produces a runtime-format word; OpenGL has a separate path. Neither is canonical RGB24 byte order. |
| `output_opengl.cpp:785–810` | 1,124 | `ef5b35360e35e0c0b41504ba4fd20379a836cb25f3287b08c41281ebd33f0412` | GL_RGBA8 allocation and BGRA upload describe texture storage, not a completed RGB24 readback. |
| `output_opengl.cpp:1030–1094` | 2,697 | `d0845c1406aefa296a86fae4d0beed19c61e5cf616f8af2ebb986bbda3dd8d7e` | PBO and direct upload branches need separate runtime selection and conversion provenance. |
| `sdlmain.cpp:10223–10225` | 69 | `b54768a51e3e65c951ad52e418647e8558a09450872da9c9c64c34baf46b8311` | The SDL2 OpenGL swap wrapper is void; a call cannot itself report success. |

## Completion criterion: still missing on the selected trace

The traced `RENDER_EndUpdate` path can call `CAPTURE_AddImage` before backend
dispatch, then call `GFX_EndUpdate` only for selected output conditions.
`GFX_EndUpdate` can return before a backend call or choose a branch that does
not present. Surface and GL backend functions contain additional skip paths.
The selected source windows expose candidate presentation calls, but no
independent, same-frame **successful output** receipt after them. A returned
integer from a surface API, even if a future hook retains it, would establish
an API result; it does not by itself authenticate a completed host frame.
The GL swap wrapper has no result to retain. The current synthetic backend
classifier's `completion_verified` flag checks a supplied witness, not the
origin of that witness.

A future completion producer must first name the selected backend/runtime
format and presentation site. It must then source the call outcome where one
exists, a separate successful-output criterion, and a unique backend event
identity tied to the **same new frame** and common issuer order. A no-output,
failed, counter-only, upload-only, or unverified attempt cannot mint a
completed backend event for RGB association. The exact independent criterion
and its source site remain **unidentified** in these traced windows; this
note does not assert that no criterion exists elsewhere.

## Canonical RGB24 conversion: required source fields

The future producer must record the reached source palette translation and
frame identity, selected scaler/aspect and clip, backend type, surface depth,
bytes per pixel, pitch and R/G/B masks/shifts **or** the selected GL upload
and readback path. It must identify the actual post-completion buffer and
convert that buffer into top-left-origin, row-major RGB bytes, three bytes per
pixel, with an explicit stride and exact extent. Surface words from
`SDL_MapRGB`, source-cache capture bytes, and BGRA GL uploads are different
representations. Their digests cannot be substituted for a digest of the
canonical RGB24 bytes. A 15-bit surface and padded/clipped surfaces require
the reached byte width and row origin; OpenGL texture content requires a
separate readback and channel/orientation account.

The existing v2 RGB24 parser can validate **supplied** bytes, stride, extent
and SHA-256. It cannot establish their backend conversion, post-completion
timing or frame association. For the saved row, source x `8..295`, y
`112..119` reaches host crop `(16,224,576,16)` only if the same frame proves
the conditional 320×200 uniform 2× geometry, page, palette, no split/skip,
and no overlay. The fixed expected patch is 27,648 bytes with SHA-256
`e6f630a6abf12a030e92387a2966f7ab15abff63f8a8a57b9b8988e516dff980`;
it is not an observed host frame.

Fail closed for pre-backend capture, skipped renderer/backend paths, surface
no-change or zero rectangles, texture upload without completed presentation,
void swap alone, ignored API result, unresolved runtime format, inferred
channel order, stale or old-frame completion, and RGB bytes with only a
matching digest or label. Status:
**BLOCKED_INDEPENDENT_BACKEND_COMPLETION_AND_CANONICAL_RGB24_PRODUCER**.
No native timing, first writer, selected page, RGB parity, frame parity, or C
parity follows. Private source, media, palette, plane bytes and images stay
under ignored `resources/`.
