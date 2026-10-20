# Declared boundary: `RES_0AF8_16BC` (DOS `INT 21h` AH=30h + environment copy)

**Status: declared boundary contract — USABLE as a dependency, NEVER a credit.**
Never appears in `strict_full_c_addresses.csv` or `reviewed_function_identities.csv`;
the queue loader refuses that and `test_strict_candidate_queue` proves it.

## Extent
`0AF8:16BC`, **79 bytes**, `RETF 2`. Loader-expanded image, SHA-256 `42c78869…`.

## What it is
Two modes selected by the first argument `[bp+6]`:
- **non-zero** → `call 1712` (a SYSTEM helper inside the same segment) and return
  that result;
- **zero** → `mov ah,30h; int 21h` (DOS get-version), then walks the environment
  far pointer at `ES:[0x2C]` looking for the version string, measures it with
  `repne scasb`, and copies length-prefixed text into the caller's buffer at
  `[bp+8]` with `stosb`/`rep movsb`, honouring `DS`/`ES` via a `push ds; push es`
  swap and `pop ds`.

## Modelled
- Argument dispatch on `[bp+6]`, the `RETF 2` frame, and the buffer write contract
  (length byte then text) for the zero-argument path.
- The `ES:[0x2C]` environment-pointer dereference and the copy loop.
- `AL`/`CX`/`DI`/`SI` results the caller observes.

## NOT modelled
- The DOS service itself: no DOS kernel, no real environment block, no version
  value beyond what the measured execution returns.
- The `1712` helper's own semantics — it is inside this declared boundary.
- Interrupt vectors, TSR interference, and any environment mutation.

## How behaviour is supplied
The differential: the original bytes execute under Unicorn/DOSBox-X against the
same RAM image (including the environment pointer the fixture installs), and the
caller's C model is compared over all 14 CPU words and the full 1 MiB.

## Limits
- Composable, not credited.
- A composing caller records the boundary in its own evidence document.
- The non-zero path delegates to `1712`; a future translation must preserve both
  modes, not only the version/copy path described above.
