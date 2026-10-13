# Saved slot-A view-four: first later-group tile effect

The first non-group-zero item in the reached view-four placement is its ninth
tile call. The saved route reaches `2D14:02FB`, which calls the overlay
`0x428:002A` entry-2 dispatcher, and returns to `2D14:0300`. This note
models only the destination object's masked plane-byte effect at that call.
It is a **bounded model**, not a complete entry-2 or resident compositor
translation. No full-C count, game-identity count, displayed-frame, or
whole-view parity claim follows from this slice.

Before debugging, `deja "Poolrad slot A view four first non-group-zero tile
effect 0x40C 0x3BD callback"` returned no exact session. The broader
`deja "Poolrad 0x40c draw"` recalled the font-loader lead. Reused and
independently checked: the merged
[first tile-plane note](SLOT_A_U40C_FIRST_TILE_PLANE.md) for the same entry
and resident compositor, the [first-wall handoff](SLOT_A_U3BD_FIRST_WALL_TILE.md),
and the runtime Move characterization in
[the row-56 transfer note](SLOT_A_U40C_ROW56_TRANSFER_MODEL.md).
The view-four caller model and first-three callback capture in PR #342
provided the exact next-call boundary; this audit repeated the native run
and captured call nine directly.

## Original code and reached path

Ignored `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x40C` entry 2 at file `[0x32392,0x324E2)` is 336 bytes,
127 aligned instructions, SHA-256
`2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00`.
At `+0x0196..+0x01A4`, the ninth item selects group three
(`0xBA..0xFF`). `+0x01F9..+0x0203` sees a present source object,
`+0x020F..+0x0213` subtracts the group-three remap word, and the
nonzero flag takes the `+0x0249` far call to `RES_0464:08D4`.
The original ninth stack tuple at `2D14:02FB` is
`(x=4,y=7,item=0xE2,flag=1,other=1)`; remap `0xBA` yields index 40.

Ignored `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
`RES_0464:08D4..0D71` is file `[0x4F4D,0x53EA)`, 1,181 bytes,
429 aligned instructions, SHA-256
`9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe`.
Its auxiliary-present byte loop `+0x0BCA..+0x0CBF` is file
`[0x5243,0x5338)`, 245 bytes, 86 aligned instructions, SHA-256
`fa187400915013d150d014e7200c4a45104c9f389b02859479281f8fefb492b2`.
The live source auxiliary far pointer has offset zero and a nonzero
segment; the OR of both words at `+0x08E3..+0x08EE` therefore selects the
`+0x0BD3` masked loop. Treating the zero offset alone as a null pointer
would select the wrong branch. Video state is one, source dimensions are
8 rows by 1 byte-column, source item stride is 32 bytes, and destination
dimensions are 168 rows by 21 byte-columns. For each of eight rows and four
plane bytes, `+0x0C3D..+0x0C6A` writes
`(old_destination & auxiliary_mask[row]) | source[item,row,plane]`.

## Immediate native/C comparison

A fresh sandboxed, hash-verified saved-slot-A run stopped immediately
before the ninth far call at `2D14:02FB` and immediately after it at
`2D14:0300`. Both stops captured all 655,360 guest RAM bytes under ignored
`resources/`. The destination object pointer stayed at physical `0x3DE48`.
Its 14,135-byte pre-call SHA-256 is
`0c1d5eb4b3b1d9aebe59bfef530b213b5d21699c4f513108152a52dd1d8efbde`;
its immediate post-call SHA-256 is
`741ba5aefece6c49046364fae12c7f0d8e75551f45a749edfac65dd35f78bd5a`.
The native call changed 20 object bytes, at offsets `0x128A..0x14D6`.

The private C probe fed the pre-call object, group-three source, resolved
auxiliary mask and tuple to `pr_u40c_entry2_group3_tile_plane`. It compared
the **entire immediate post-call object: 14,135/14,135 bytes match**.
The model reports 32 byte writes and 20 changed bytes. A private paired
native/C/pixel-index-difference PNG under
`resources/oracle-runs/slot-a-later-tile-p5-008/` has SHA-256
`39a339b7d4ea110e59a5eba4cd4940ad41be188d8718258e88f20441829b1b8c`;
its diff panel is empty. That image uses surrogate grayscale for the
offscreen EGA indices; it is not a DOS screenshot or a displayed frame.

The C boundary includes group-three selection, remap, the reached flag and
video mode, auxiliary pointer presence, object shapes and the destination
plane-byte loop. Host span checks precede all writes. Original dirty-row
and extent metadata, other groups or video modes, other tile calls, and
the later display transfer remain outside this model.

## Reproduction

```sh
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
for path, start, end in (
    ('GAME.OVR', 0x32392, 0x324E2),
    ('START.EXE', 0x4F4D, 0x53EA),
    ('START.EXE', 0x5243, 0x5338),
):
    data = Path('resources/DOS/POOLRAD', path).read_bytes()[start:end]
    print(path, hex(start), hex(end), len(data), sha256(data).hexdigest())
PY
cmake -S . -B resources/build-slot-a-later-tile-p5 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-later-tile-p5 \
  --target poolrad_overlay_u40c_group3_tile_plane_tests
ctest --test-dir resources/build-slot-a-later-tile-p5 \
  -R '^poolrad_overlay_u40c_group3_tile_plane_tests$' --output-on-failure
```

All original executables, saves, source objects, RAM captures, and derived
images remain in ignored `resources/`.
