# Saved slot-A view-four callback four: unchanged plane bytes

The saved view-four placement reaches its fourth `0x428:002A` tile callback
at `2D14:02FB`, then returns at `2D14:0300`. Its tuple is
`(x=5,y=4,item=1,flag=1,other=1)`. This is the lowest-index callback after
the first three modeled in the [view-four placement note](SLOT_A_U3BD_VIEW4_PLACEMENT.md).
The reached group-zero tile-plane model performs 32 plane-byte writes and
matches the entire immediate native destination object, with **zero changed
bytes**. The sampled A200 and A000 EGA planes also have zero immediate
changes. This is one bounded callback observation, not a complete view-four
render or displayed-frame parity claim. No full-C or game-identity count is
claimed.

Before debugging, `deja "Poolrad saved slot A view four tile callback sequence
auxiliary mask A200 A000 #346 #349 lowest unmodeled"` found no indexed
session. Reused and checked the merged [view-four placement
walk](SLOT_A_U3BD_VIEW4_PLACEMENT.md), [group-zero plane-byte
model](SLOT_A_U40C_FIRST_TILE_PLANE.md), and the [later group-three
tile](SLOT_A_U40C_GROUP3_TILE_PLANE.md) and [metadata
audit](SLOT_A_U40C_GROUP3_DIRTY_METADATA.md) as route and ABI leads. The
fourth call was captured anew; the ninth and tenth calls in those later
notes do not establish this call's effects.

## Pinned code and reached arguments

Ignored `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The caller's local `+0x0211..+0x031F` is file `[0x2EB08,0x2EC16)`, 270
bytes and 108 aligned 16-bit instructions, SHA-256
`b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03`.
At `+0x02F2..+0x02FA`, the caller pushes the item and two one-valued
flags; `+0x02FB` is `LCALL 0x428:002A`, followed by `+0x0300`.
The entry-2 dispatcher file `[0x32392,0x324E2)` is 336 bytes, SHA-256
`2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00`.
Independent 16-bit decode shows `+0x0163..+0x0174` accepting item 1 in
group zero, `+0x020F` loading the remap, `+0x0213` subtracting it,
`+0x0216..+0x021A` testing the nonzero flag, and `+0x0249` calling
`0x464:08D4`.

Ignored `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The resident compositor `RES_0464:08D4..0D71` is file `[0x4F4D,0x53EA)`,
1,181 bytes, SHA-256
`9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe`.
Its auxiliary-present byte loop is file `[0x5243,0x5338)`, 245 bytes,
SHA-256 `fa187400915013d150d014e7200c4a45104c9f389b02859479281f8fefb492b2`.
At `+0x08E6..+0x08EE`, the original ORs both far-pointer words before
selecting the auxiliary route. At `+0x0C58..+0x0C6A`, it reads the old
destination byte, ANDs the auxiliary byte, ORs the source byte, and stores
the result at the destination object's data field `+0x17`.

The native call frame at `SS:SP=1D16:3E78` carries item 1 at `SP+4`, y 4
at `SP+6`, and x 5 at `SP+8`; both preceding flag words are 1. Group-zero
remap `DS:[0x2722]=1` selects item index 0, and video state
`DS:[0x4A1E]=1` selects the masked plane-byte route. The live source far
pointer `DS:[0x5E3A]` resolves to physical `0x417B0`; the source header's
auxiliary far pointer `+0x13/+0x15` resolves to `0x427C8`. The live
destination `DS:[0x4A20]` resolves to `0x3DE48`. Source dimensions are
8 rows by 1 column with 32 bytes per item; destination dimensions are
168 rows by 21 columns. The fourth tile begins at destination data offset
`((4*8)*21+5)*4 = 2708`, after the 23-byte header.

## Immediate native and C evidence

The hash-verified sandbox replay stopped before and after this exact far
call. Its ignored full 640 KiB RAM captures are under
`resources/oracle-runs/slot-a-view4-call04-p5-002/`:

| Stop | Full RAM SHA-256 | 14,135-byte destination SHA-256 |
| --- | --- | --- |
| `2D14:02FB` | `f2ebc54b74b63ab515a8ccebff919425d25dcffc6601da0183bb447995ffd0a6` | `92075358a41fa616b54e7fb3bcfd6d278cd36e58a4f5ff00b042dec3cf27f485` |
| `2D14:0300` | `4f73f23c049b2bc6246d66bbab7defd2e90bf96530822d22ff510b761b9feb92` | `92075358a41fa616b54e7fb3bcfd6d278cd36e58a4f5ff00b042dec3cf27f485` |

The selected eight auxiliary rows preserve existing destination bits,
and item-index-zero contributes no set source bits across its 32 plane
bytes. A private probe passed the captured pre-call source, auxiliary mask,
destination, remap, video state, and tuple to the merged
`pr_u40c_entry2_mode1_tile_plane`. It returned `OK`, index 0, data offset
2708, 32 writes and zero changed bytes. Its complete 14,135-byte output
matched the native post-call object. Across all guest RAM, only nine bytes
changed, within the transient stack span `SS:3E41..3E7C`.

Four EGA read-map planes were dumped at each stop for the first 8,192
bytes of both `A200` and `A000` windows. All 65,536 sampled bytes matched
pre/post. Concatenated four-plane SHA-256 was unchanged: A200
`c35020473aed1b4642cd726cad727b63fff2824ad68cedd7ffb73c7cbd890479`;
A000 `8fa4f755484abda61c75dbf6aba7b7dadca4f70906b368a7c2bdc8c7c74dadaf`.
The observation covers these sampled windows at the immediate return,
not a later display flush or the rest of the aperture.

The synthetic test composes the existing view-four placement callback with
the existing group-zero tile-plane model. It checks the fourth tuple and
whole-object preservation after three prior callbacks, then changes the
synthetic auxiliary mask and source to make four plane bytes change at the
same address. This tests the effect rule even though the captured fourth
call is an identity operation.

## Reproduce the public checks

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
for name, lo, hi, expected in (
    ('GAME.OVR', 0x2EB08, 0x2EC16, 'b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03'),
    ('GAME.OVR', 0x32392, 0x324E2, '2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00'),
    ('START.EXE', 0x4F4D, 0x53EA, '9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe'),
    ('START.EXE', 0x5243, 0x5338, 'fa187400915013d150d014e7200c4a45104c9f389b02859479281f8fefb492b2'),
):
    span = (Path('resources/DOS/POOLRAD') / name).read_bytes()[lo:hi]
    assert sha256(span).hexdigest() == expected
    print(name, hex(lo), hex(hi), len(span), expected)
PY
cmake -S . -B resources/build-slot-a-view4-tile04-p5 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-view4-tile04-p5 \
  --target poolrad_slot_a_view4_tile04_tests
ctest --test-dir resources/build-slot-a-view4-tile04-p5 \
  -R '^poolrad_slot_a_view4_tile04_tests$' --output-on-failure
```

Original media, RAM captures, EGA planes, and any image output remain under
ignored `resources/`.
