# Saved slot-A view: reached view-four placement

After the view-three `+0x0A7A..+0x0AA9` handoff, unit `0x3BD` entry 5
reaches its `+0x0AA9..+0x0AD8` view-four gate. The first two scan
iterations skip the local placement. The third reaches `+0x0AD5`, calls
local `+0x0211`, and returns at `+0x0AD8`. This bounded C model reproduces
that gate and the local piece walk up to the `0x428:002A` callback. The
first three tile callbacks and the full offscreen object after each have
private native/C parity; the third is the first observed object change.
The later tile groups, rest of entry 5, display transfer, and visible frame
remain outside this model.

Before tracing, `deja "Poolrad slot A unit 0x3BD after +0x0AA9 view three
next art placement branch callback"` was run and timed out. Reused and
checked the merged [nibble lookup](U3BD_NIBBLE_ENTRY2.md),
[first-wall placement](SLOT_A_U3BD_FIRST_WALL_TILE.md),
[view-nine placement](SLOT_A_U3BD_VIEW9_OLD_WALL.md), and
[group-zero tile-plane model](SLOT_A_U40C_FIRST_TILE_PLANE.md) for the
entry-2 selector, `+0x0211` walk, callback ABI, and first tile-effect
comparison. The pending view-three PR is contextual evidence; this C file
does not depend on its source.

## Code and reached decision

Ignored DOS `GAME.OVR` SHA-256:
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x3BD` file base is `0x2E8F7`, loaded segment `2D14`. The half-open
branch `+0x0AA9..+0x0AD8` is file `[0x2F3A0,0x2F3CF)`, **47 bytes / 22
aligned 16-bit instructions**, SHA-256
`6ed80e2731339cb9141dcc04cd06a00c95e9069235203b73ee73303b7b97cb0c`.
Its existing local `+0x0211..+0x031F` is 270 bytes, SHA-256
`b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03`.

`+0x0AA9..+0x0AB6` passes the low bytes of `[BP-0x0D]` and `[BP-0x0F]`
with facing byte `[BP-1]` to entry 2 at `+0x047B`; unlike the adjacent
view-three branch, this selector is **not** `[BP+6]`. The equal jump at
`+0x0AC0` skips placement on a zero nibble. A nonzero result is stored
at `[BP-0x15]` and passed to `+0x0211` with view number 4, origin word
`DS:[0x0CB2]+[BP-0x12]`, and y byte `DS:[0x0CC6]`. The local helper uses
`DS:[0x0C90]` as the slice offset and `DS:[0x0C9A]/[0x0CA4]` as
columns/rows, narrows the origin to signed bytes, clips outside 0..10,
and advances its piece cursor even when no callback occurs.

The private saved-slot-A replay stopped at `2D14:0AD8` for scan
iterations 0 and 1 via the zero gate, then at `2D14:0AD5` for iteration
2. That reached frame has a nonzero nibble, a 2-column by 8-row view-four
slice, and sixteen positive selected cells. The first three
`2D14:02FB` tile-call frames carry the `+0x0AD8` return word. The replay
subsequently hit the saved gameplay return `0x1C:+0x3387`.

## Offscreen and C parity boundary

The offscreen object is 14,135 bytes, including its 23-byte header. All
private full-RAM captures remain under ignored `resources/`:

| Stop | Full-RAM SHA-256 | Offscreen-object SHA-256 |
| --- | --- | --- |
| First zero gate at `+0x0AD8` | `a5080458fedcb618628e6387c692141a24b1b1073b9cac7b86fdc86a76040fa7` | `e2a8d70f54501d664f3d47e50e0b614d563f36dbc3cde6eb15de6210d1c0ce94` |
| Third iteration before `+0x0AD5` | `50f0dfe340697048d8fefeb0b89bab79b68e343fabe79acb20260ac15ab576e3` | `f54d5c29e66e399b7bf9ceae7685c1b83e2a00fd15cb55a7dd771a48a9c48e3d` |
| After the third tile callback | private capture | `92075358a41fa616b54e7fb3bcfd6d278cd36e58a4f5ff00b042dec3cf27f485` |
| After placement returns at `+0x0AD8` | `a271faef6a114cded5df2c4bb30241580a93e563f4233f869df846c1f0a0225e` | `8664637ddd0016f3ef9978c4694f87ecbb7fb325ab147475f936c410ce9c6e8d` |

The first two tile callbacks leave the object unchanged. The third changes
14 object bytes at offsets `0x0AFD..0x0CF6`. Across the complete
view-four placement, 244 object bytes differ from the pre-call capture;
this aggregate includes later tile groups that this PR does not model.

A private C probe read the original frame and caller-owned GEO/piece
tables. The new view-four model matched the **first 3/3 ordered native
`(x,y,item,1,1)` call frames**; it deliberately stopped before the fourth
unrecorded callback. Passing those three calls into the merged group-zero
tile-plane model matched the **entire 14,135-byte native object after each
call**. The first two changed zero bytes; the third changed fourteen. A
paired native-reference/C/diff PNG was rendered from those private object
bytes in surrogate grayscale EGA indices; its diff panel is empty. The
PNG SHA-256 is
`5c671d9341c39c8f4d1ebf80d445a354499d1411269942ae42e91e161b73d74b`.
It is a partial **offscreen object** comparison, not a DOS screen capture
or complete frame parity. The saved slice has eleven group-zero items and
five other positive items, so the merged tile-plane model cannot account
for the whole `+0x0AD8` object.

The public C test uses synthetic GEO/piece bytes to exercise the positive
walk, zero gate, clipping, reached bounds after a callback, absent/failing
callback, unsupported selector residue, and word-to-byte narrowing. Original
media, RAM captures, model inputs, and the paired image remain ignored.

## Reproduce the public checks

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
b = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()
assert sha256(b).hexdigest() == '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'
lo, hi = 0x0AA9, 0x0AD8
span = b[0x2E8F7+lo:0x2E8F7+hi]
ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, lo))
assert len(span) == 47 and len(ins) == 22 and ins[-1].address + ins[-1].size == hi
print(sha256(span).hexdigest())
PY
cmake -S . -B resources/build-slot-a-view4-p5 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-view4-p5
ctest --test-dir resources/build-slot-a-view4-p5 --output-on-failure
```
