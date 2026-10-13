# Saved slot-A first view: reached mode-nine old-wall placement

This note resolves the next two native placement calls after the first
front-wall pair. DOS `GAME.OVR` unit `0x3BD` entry 5 reaches its
`+0x0744..+0x0795` old-wall branch, selects local view 9, and calls the
existing `+0x0211` placement helper. The bounded C model reproduces that
branch and its two `0x428:002A` handoffs. It does not translate all of entry
5, assign a wall's visual meaning, or claim displayed-pixel parity.

Before analysis, `deja "Poolrad slot A static view wall object placement
geometry parity first view"` found no indexed match. Reused and checked:
the [first front-wall handoff](SLOT_A_U3BD_FIRST_WALL_TILE.md) established
the call ABI and piece-table selection, the [entry-2 nibble lookup](U3BD_NIBBLE_ENTRY2.md)
established the facing-4 high-nibble rule, and the
[first tile plane-byte model](SLOT_A_U40C_FIRST_TILE_PLANE.md) established
the reached offscreen write boundary. This trace independently captures
the next two original call frames and checks that model against them.

## Pinned span and reached gates

Ignored `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x3BD` begins at file `0x2E8F7`. The half-open branch span
`+0x0744..+0x0795` is file `[0x2F03B,0x2F08C)`, 81 bytes, SHA-256
`21cc02eccc6dae5289541919c4b1e36102f417b4ca24c5c84402ff607c8faec6`.
Capstone 5.0.7 decodes 38 aligned 16-bit instructions ending at `+0x0795`.
The separately called local `+0x0211..+0x031F` span is 270 bytes, SHA-256
`b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03`.

The native replay begins at saved gameplay `0x1C:+0x3382` and breaks at
entry-5 local `+0x02FB` and return `+0x0795`, then reaches gameplay
`0x1C:+0x3387`. The first two `+0x02FB` calls reproduce the previously
documented mode-zero pair `(x,y,item)=(7,6,1),(7,7,13)`.
The next two frames return to entry 5 `+0x0795`, proving they came from
the `+0x0792` call rather than the first-wall `+0x073F` call.

At `+0x0744`, old-wall byte `[BP-0x17]=14`, so the `jbe +0x0795` gate
does not skip. The reached frame has front word coordinates `(13,5)`,
selector byte `[BP-1]=4`, and its facing-indexed signed step `(0,1)`.
Instructions `+0x074A..+0x0777` subtract that step and pass `(13,4,4)`
to the merged nibble helper. It reads the **high** nibble at decoded GEO
offset `0x100+16*4+13=0x14D`, yielding 15. Thus the zero-test at
`+0x077A` falls through. This sampled 15 is only a nonzero gate:
`+0x078A` pushes the *previous* wall 14 to the placement helper.

The origin expression at `+0x077E..+0x0792` is
`DS:[0x0CBC]+[BP-0x12]+1 = 5+(-2)+1 = 4`; y is
`DS:[0x0CD0]=4`. Local `+0x0211` reads view-9 table values offset 154,
one column, two rows. The selected piece positions are
`(14-1)*156+154=2182` and 2183. Their reached item IDs are 1 and 13.
After the local `+2` grid shift, the native `0x428:002A` calls carry
`(x,y,item,1,1)=(6,6,1,1,1)` and `(6,7,13,1,1)`.
These are tile-grid arguments, not proven screen-pixel positions.

## Native and C parity boundary

The ignored full-RAM captures have SHA-256 values:

| Stop | SHA-256 |
| --- | --- |
| Before third tile call | `32497ab52d24e0418149a7b6dd24057085563359d58918526a0a1c6b7aec1cb1` |
| Before fourth tile call | `4375b3b634d9c86dd3118f24b2e2f25ffdb6f77905e602128e820835da40de53` |
| After fourth tile, at `+0x0795` | `5a47979ca6b86136ddb19e4a0a5ac3c53867dd8e138b606d9d4beda27a5362fe` |
| Gameplay return `+0x3387` | `5827dc31f58636a38ef8fc58295377bddce8ffe822a190083ae5ff259fc0e751` |

A private strict C probe extracted the entry-5 frame, GEO/piece pointers,
and table values from the first capture. `pr_u3bd_entry5_view9_wall`
matched **2/2 native call tuples** without altering the buffers. It then
passed each tuple and its pre-call objects to the merged
`pr_u40c_entry2_mode1_tile_plane`; the entire 14,135-byte destination
object matched the corresponding post-call capture for both calls.
The first tile changed zero data bytes; the second changed nine at
offscreen data offsets `0x127A..0x14C4`. The data SHA-256 changed from
`324a9afc43190eaaa020a22ab57c6f551fa1409c1c33c548b88a47ca71ea4da3`
to `80f2e4085effe0b29a45c6c44baea8ce24d57f407040b4e279380bc47901f819`.
The object changed again before gameplay `+0x3387`; later composition and
the display transfer remain unassigned by this pair.

The C model starts at `+0x0744`. It stops before GEO access when old wall
is zero, skips the local draw when the sampled nibble is zero, narrows
DOS word coordinates to the original byte ABI, and walks view 9 with
the local signed-byte clipping and piece cursor. It reports reached
buffer or callback failures explicitly. Synthetic tests cover the two
tile handoffs, both zero gates, clipping before piece reads, zero pieces,
reached bounds, absent draw hook, unsupported selector residue, and word
truncation. Original GEO, art, saves, captures, and pixel data remain in
ignored `resources/`.

## Local verification

```sh
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
p = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()
for start, end in ((0x2F03B, 0x2F08C), (0x2EB08, 0x2EC16)):
    data = p[start:end]
    print(hex(start), hex(end), len(data), sha256(data).hexdigest())
PY
cmake -S . -B resources/build-slot-a-view9-p5 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-view9-p5 \
  --target poolrad_overlay_u3bd_view9_wall_tests
ctest --test-dir resources/build-slot-a-view9-p5 \
  -R '^poolrad_overlay_u3bd_view9_wall_tests$' --output-on-failure
```

The replay comparison is private because its full RAM and art inputs
must not enter the repository. The model is a bounded reached path, with
the `0x428:002A` and eventual screen effects delegated to separate work.
