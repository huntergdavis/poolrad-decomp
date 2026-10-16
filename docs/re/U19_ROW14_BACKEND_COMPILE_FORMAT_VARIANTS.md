# U19 row-14 backend compile and upload format variants

This source-only follow-on refines the merged
[backend RGB24 format boundary](U19_ROW14_BACKEND_RGB24_FORMAT_SOURCE.md).
It separates SDL2 window-surface setup, the SDL1 surface-copy branch, and
OpenGL texture allocation/upload. None of these compile branches selects the
saved run's backend or converts an observed frame to canonical RGB24. A bounded
`deja "Poolrad U19 DOSBox-X SDL1 SDL2 OpenGL backend pixel masks BGRA GL_RGBA8
RGB24 frame conversion"` query returned no matching indexed session; the
merged format boundary supplied the runtime format and pitch lead.

## Original-source lock

The ignored DOSBox-X source revision is
`5a9c7c53481c2e933948039c33256776653f4faf`. Full-file SHA-256 values
are `60670d41fe31cb6ecf65343bd540beaae93d9e0c39f27495fd63a70c1cd4067e`
for `output_surface.cpp` and
`6b4f203681fd19b7c9b6b17a99d94ad5a95e1cddade9372d49a93d0c02fd4a54`
for `output_opengl.cpp`. These inclusive line windows retain original newlines:

| Source window | Bytes | SHA-256 | Conditional fact |
| --- | ---: | --- | --- |
| `output_surface.cpp:54–106` | 2,121 | `1918a955d5afb73ede341a18577f7f0bbf8af704601fc250fcffcb0334da0344` | `C_SDL2` guards the first surface implementation. |
| `output_surface.cpp:178–209` | 1,369 | `d889550125663aa9308f1f98ec1295b34759517415304495e5e1fc4c679d6bde` | SDL2 retrieves a runtime window surface and switches on its `BitsPerPixel`; channel layout is not fixed by this code. |
| `output_surface.cpp:245–252` | 137 | `67b036858b2bce4df78877af5d2a6c3a8fbb89124c4aee5c4d5ab9e8a4de3eb9` | The `#else` begins the separate non-`C_SDL2` implementation. |
| `output_surface.cpp:532–565` | 1,196 | `a4a4b070ea19824b201f2ad73f023b07620b47cc7ff2ac16e85200f9a1f8c2b5` | Under SDL1, the `SDL_DOUBLEBUF` guard permits a blit surface created with `SDL_HWSURFACE`; it copies the runtime surface's bit depth and R/G/B masks into `SDL_CreateRGBSurface`. This is not the SDL2 window-surface path. |
| `output_surface.cpp:590–599` | 226 | `f8739d2ea2b038eda20f2adf46d8526ab5ecfc0698800124c9a7322079fefbbc` | The non-`C_SDL2` implementation closes before common start-update code. |
| `output_opengl.cpp:785–810` | 1,124 | `ef5b35360e35e0c0b41504ba4fd20379a836cb25f3287b08c41281ebd33f0412` | OpenGL allocates a `GL_RGBA8` texture using `GL_BGRA_EXT`, `GL_UNSIGNED_BYTE`, and null initial data; allocation is not an RGB24 capture. |
| `output_opengl.cpp:1030–1094` | 2,697 | `d0845c1406aefa296a86fae4d0beed19c61e5cf616f8af2ebb986bbda3dd8d7e` | Normal PBO/full and changed-line uploads use `GL_BGRA_EXT`; element type follows branch-specific platform guards. A no-change path returns before upload. |

The SDL1 mask-copy call inherits its selected surface's masks; the SDL2
window-surface path must inspect the surface returned at runtime. The OpenGL
internal texture format, upload format and element type describe different
stages, and the xBRZ upload path is separate from the normal window above.
No packed word or texture declaration establishes RGB24 channel order in a
host capture. A future producer must first identify the compiled SDL branch
and selected backend, then retain the runtime width, height, pitch, bit depth,
bytes per pixel, masks/shifts or GL upload type, clip/scaler path, and a
separately proved conversion to RGB24 for the same completed frame. Backend
completion and receipt sourcing are a separate local review gate; this note
does not supply their proof.

The merged v2 completed-backend parser validates **supplied** RGB24 bytes and
declarations but does not source these backend properties. No selected
backend, event clock, page, frame, RGB value, native pixel parity, or C parity
is established. Reproduce this audit by hashing the complete line windows
above in the ignored source revision. Original source copies, media and image
buffers remain under ignored `resources/`.
