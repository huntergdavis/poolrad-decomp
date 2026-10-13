# Saved slot-A view: reached view-three placement branch

The restored slot-A static-view route reaches unit `0x3BD` entry 5's
`+0x0A7A..+0x0AA9` branch after its view-two scan. This bounded C model
reproduces the nibble gate and the existing local `+0x0211` piece walk up to
four calls to `0x428:002A`. It does not translate entry 5 as a whole, the
callee's art effects, or the later view branches. The four calls have private
native/C argument parity; no original image bytes appear here.

Before implementation, `deja "Poolrad slot A unit 0x3BD +097F next view-one
art geometry placement return +3387"` was attempted through both the CLI
and recall service; both stalled. The branch was independently byte-checked
against the ignored DOS binary. Reused the merged [first-wall placement](SLOT_A_U3BD_FIRST_WALL_TILE.md)
and [view-nine placement](SLOT_A_U3BD_VIEW9_OLD_WALL.md) for the `+0x0211`
and `0x428:002A` ABI, and the merged [nibble lookup](U3BD_NIBBLE_ENTRY2.md)
for the selector behavior. The pending view-one scan PR is context only,
not a code dependency.

## Pinned code and route

Ignored DOS `GAME.OVR` SHA-256:
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x3BD` file base is `0x2E8F7`, loaded segment `2D14`. All intervals
below are half-open and Capstone 5.0.7 decoded them on exact boundaries.

| Unit span | File interval | Bytes / instructions | SHA-256 | Role |
| --- | --- | --- | --- | --- |
| `+0x097F..+0x0A16` | `[0x2F276,0x2F30D)` | 151 / 63 | `cffa2359b10e558baf9e85c444ca0524bbf21d5f7430c0c6ea719e6a8d7ca522` | Reached view-two scan; its selected placement is clipped |
| `+0x0A19..+0x0A7A` | `[0x2F310,0x2F371)` | 97 / 36 | `273127afaa4fec0c268aaddf137e9a6d66b74817a1375713bb3884bbd6ab204b` | Depth-one setup before this branch |
| `+0x0A7A..+0x0AA9` | `[0x2F371,0x2F3A0)` | 47 / 22 | `61f65a27f0a5bbfae08e76491d8c44c3d7d6a9f77598d37bca99c83b19054c1b` | Modeled gate and local call |
| `+0x0211..+0x031F` | `[0x2EB08,0x2EC16)` | 270 / 108 | `b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03` | Shared local placement helper |

In the private saved-slot replay, `2D14:097F` scans three cells. The first
two nibble results are zero; the third is nonzero and reaches `+0x09E5`,
which passes view 2 to `+0x0211`. Every selected column has x beyond the
helper's inclusive 0..10 grid, so the object hash stays unchanged through
`+0x0A16`; no tile callback occurs. The loop then reduces its depth to one,
enters `+0x0A19`, and reaches `+0x0A7A`. The saved gameplay caller later
resumes at `0x1C:+0x3387`.

At `+0x0A7A`, the routine pushes the **low bytes** of frame words
`[BP-0x0D]` and `[BP-0x0F]` plus selector byte `[BP+6]` into entry 2 at
`+0x047B`. The latter is the outer view selector, not facing `[BP-1]`.
`+0x0A8D` skips to `+0x0AA9` on a zero nibble. A nonzero nibble selects
that wall's 156-byte piece slice. `+0x0A93..+0x0AA6` passes
`DS:[0x0CB0]+[BP-0x12]`, `DS:[0x0CC4]`, the nibble, and view 3 to
`+0x0211`. The helper reads the low byte of origin x/y as signed cells,
uses `DS:[0x0C8F]` for the slice offset and `DS:[0x0C99]/[0x0CA3]`
for columns/rows, advances its piece cursor across clipped cells, and calls
`0x428:002A` only for nonzero items within the grid. Those four call
frames return through `2D14:0AA9`.

## Native/C boundary

The replay stopped at every `2D14:02FB` handoff reached by `+0x0AA6`.
Each frame has return word `+0x0AA9`; an ignored private C probe read the
captured frame, GEO/piece pointers, and view metadata, then compared the
model's ordered `(x,y,item,1,1)` callbacks against the four native stacks:
**4/4 matched**, with 12 scanned cells. Public tests use synthetic GEO and
piece values. The model's draw callback is the boundary to `0x428:002A`;
it does not claim parity for that callee's offscreen changes.

Private full-RAM capture hashes identify the stops without publishing data:

| Stop | SHA-256 |
| --- | --- |
| View-two entry `2D14:097F` | `f929cd194f8f8a25630300912bbf80f41c061b6ec1053f71e7148b17dcd00036` |
| View-two exit `2D14:0A16` | `6f2283b0f580cf2125d016171819c02d36426647af734a9a3ea0d8c6c883fa21` |
| First view-three tile handoff | `6f580d8efda4efac89e11dcf1279117a21858bbb88993c48cb9236bea97355e3` |
| Fourth view-three tile handoff | `a74235dbf8599f4222d4ca6731a21e83e331827c71083d7226a1b83eb7a433ae` |
| View-three return `2D14:0AA9` | `415a784085e65b78b89e08987ea04ad0534add878e8c7d88e39e434ea078d27b` |

The 14,135-byte offscreen object's SHA-256 is
`15ef1f9670025967b98c18778f0e9cb0d5e6ca96b77b48e12e92a5ff57d9ffbc`
at both view-two entry and exit, and
`e2a8d70f54501d664f3d47e50e0b614d563f36dbc3cde6eb15de6210d1c0ce94`
at the view-three return. The changed object is an observed downstream
effect, not produced by this branch model.

The synthetic test covers a positive four-call walk, zero nibble, all
columns clipped without a piece read, reached bounds after one callback,
missing and failing callbacks, unsupported selector residue, and DOS word
to byte narrowing. A complete static C frame still needs later entry-5
branches and the relevant tile-effect groups.

## Reproduction

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
b = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()
assert sha256(b).hexdigest() == '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'
for lo, hi in ((0x097F,0x0A16),(0x0A19,0x0A7A),(0x0A7A,0x0AA9),(0x0211,0x031F)):
    span = b[0x2E8F7+lo:0x2E8F7+hi]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, lo))
    assert ins and ins[-1].address + ins[-1].size == hi
    print(hex(lo), hex(hi), len(span), len(ins), sha256(span).hexdigest())
PY
cmake -S . -B resources/build-slot-a-view3-p5 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-view3-p5
ctest --test-dir resources/build-slot-a-view3-p5 --output-on-failure
```

All executable, save, GEO, art, RAM captures, and the private parity probe
remain under ignored `resources/`.
