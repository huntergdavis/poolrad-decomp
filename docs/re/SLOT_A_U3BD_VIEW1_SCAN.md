# Saved slot-A first view: three-position view-one scan

The original saved first-view route reaches unit `0x3BD` entry 5's
`+0x08E8..+0x097F` scan. Its first position selects local view 1 and
calls the existing `+0x0211` placement helper. The bounded C model
reproduces all three samples, the first/later origin distinction, and the
helper's reached `0x428:002A` tile handoffs. It does not translate all of
entry 5 or assign displayed pixel meanings.

Before analysis, `deja "Poolrad slot A 0x3BD entry5 +0865 zero current wall
next view object placement"` found no indexed match. Reused and checked:
the [first front wall](SLOT_A_U3BD_FIRST_WALL_TILE.md) established the
`+0x0211` ABI and piece-table walk; the [view-nine branch](SLOT_A_U3BD_VIEW9_OLD_WALL.md)
established the same host handoff; the [nibble entry](U3BD_NIBBLE_ENTRY2.md)
established selector-specific high-nibble reads; and the
[offscreen tile model](SLOT_A_U40C_FIRST_TILE_PLANE.md) established the
bounded downstream effect. This replay independently follows the next
reached view-one scan.

## Exact original boundary

Ignored `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x3BD` begins at file `0x2E8F7`. The half-open scan span
`+0x08E8..+0x097F` is file `[0x2F1DF,0x2F276)`, 151 bytes, SHA-256
`ecab0ea500549a1daa4b74f6e852c4f28c3340c19a43c690dc9cef7dd898eb0a`.
Capstone 5.0.7 decodes 63 aligned 16-bit instructions ending exactly at
`+0x097F`. The separately called local `+0x0211..+0x031F` is pinned in
the first-front-wall note. The preceding old-wall branch `+0x0865..+0x08BA`
is 85 bytes, SHA-256
`3af5d1fd4ecf9689eab13e72fc118b7e4517fd55e692632698b138554b7b1bf8`.

The native replay reaches `2D14:0865` with previous-wall byte 14 and
selector `[BP-2]=0`. That branch subtracts selector-zero step `(0,-1)`
from front words `(13,2)`, sampling `(13,3)` through nibble entry 2.
The selected high nibble is zero, so `+0x089D` skips the `+0x08B3`
view-nine placement. `+0x08B6` clears the previous-wall byte. The
later scan uses **different selector** `[BP-1]=4`; treating these as the
same facing would give the wrong gate result.

At `+0x08E8`, the scan copies entry-5 words `(13,4)` to its local
sample coordinates. `+0x08FD` iterates three times. Each sample passes
the low bytes of the two word locals and selector 4 to `+0x047B`, stores
AL in `[BP-0x15]`, then skips placement on zero. Iteration zero uses
`DS:[0x0CAC]+[BP-0x12]`; later nonzero iterations use the same word
expression minus one. `+0x0951..+0x097C` subtracts two from the
lateral word and advances sample x/y by the selector-4 signed steps
`(0,+1)` even after a zero wall. At `+0x097F`, the original local
state has three iterations, lateral word `0xFFFA`, and next sample
`(13,7)`.

The first sample `(13,4,4)` returns wall 15 from the selector-4 high
nibble; `(13,5,4)` and `(13,6,4)` return zero. The reached `+0x0935`
call to local placement has pushed `(origin-x,origin-y,slice,view) =
(4,3,15,1)`. View 1 selects table offset 2, one column and four rows.
The native `+0x02FB` handoff sequence is four tile-grid calls at
`(x,y,item)=(6,5,1),(6,6,18),(6,7,26),(6,8,28)`; a second replay
stopped on each call and then at `+0x097F` before any fifth call. These
are grid inputs to the graphics helper, not proven screen-pixel positions.
The scan exits at `+0x097F`
and the first view returns to gameplay `3582:3387`.

## C and private parity boundary

`pr_u3bd_entry5_view1_scan` models precisely this 151-byte local slice,
using merged nibble entry 2 and a bounded walk of local placement
`+0x0211`. It reports unsupported lookup residue, reached GEO/piece
bounds, or missing/failing tile hooks. GEO, remapped pieces, and all
graphics objects are caller-owned. The C callback order follows the
local placement's row-major cursor; clipping skips a piece read but
does not skip cursor advance.

The ignored native RAM snapshots at `+0x0865`, `+0x0916`, `+0x0935`,
and `+0x097F` have SHA-256 values respectively
`58c70f235e497bff8a6ca4091a196ca3c0b0df2402615a923e3c087c09a1e0cf`,
`48683cc05f3b6140e9123f2d9db709a2dae5fb22bf08188fc56b375722761747`,
`e0e873f2b7b02ffc4c038800ab08e96faac9f94892fabfd3da228e4236c76aa2`,
and `6531d490741a62e2adc7a520e5bbb5434231c20755fae046d084dc5f1eaa3421`.
A private strict C probe fed the captured live state to the new model.
All **four of four** C tile tuples matched direct native `+0x02FB` stack
frames from the second replay. The four private ten-byte stack snapshots
have SHA-256 values
`6f4a3fd452a0e7fc578d78704df8d42e9ed4d1ab987dae8d15210a7405d7d3e9`,
`63a2954053784035b5f88870c228e8ecdad2be8c48a6eb7194e59728b200aa00`,
`390119f42cd6958758d23c1146fa9c5b4a97ec581eb1df336fff55693a9a0782`,
and `6423c52c5e4caf8e462081e61396f014950ed61905ef3ffef76`.
The merged `pr_u40c_entry2_mode1_tile_plane` matched each first-two
native post-call **entire 14,135-byte offscreen object** and, after all
four modeled calls, the entire object at scan exit. The two independent
native scan-exit captures agree on offscreen-object SHA-256
`15ef1f9670025967b98c18778f0e9cb0d5e6ca96b77b48e12e92a5ff57d9ffbc`.
Neither captures nor media are in Git.

Synthetic tests cover the saved-shaped first placement, later-position
origin subtraction, third-position clipping, all-zero walls, reached
piece bounds, missing draw callback, an unsupported selector residue,
and GEO bounds. The scan is a bounded reached-path model, not a claim
of whole-entry-5 translation or visible frame parity.

```sh
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
p = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()
for lo, hi in ((0x2F15C, 0x2F1B1), (0x2F1DF, 0x2F276)):
    b = p[lo:hi]
    print(hex(lo), hex(hi), len(b), sha256(b).hexdigest())
PY
cmake -S . -B resources/build-slot-a-view1-p5 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-view1-p5 \
  --target poolrad_overlay_u3bd_view1_scan_tests
ctest --test-dir resources/build-slot-a-view1-p5 \
  -R '^poolrad_overlay_u3bd_view1_scan_tests$' --output-on-failure
```
