# Saved slot-A view-four callback five: masked offscreen bytes

The saved view-four placement reaches its fifth tile callback at
`2D14:02FB`, after the [fourth callback](SLOT_A_U3BD_VIEW4_TILE04.md)
has returned at `+0x0300`. Its stack tuple is
`(x=4,y=5,item=13,flag=1,other=1)`. The reached group-zero plane-byte
model matches the **entire immediate native destination object**, with
16 changed bytes. The sampled A200 and A000 EGA planes have no immediate
change. This is one callback and one offscreen effect, not a complete
view-four render, later display transfer, full-C translation, or verified
game-identity count.

Before debugging, `deja "Poolrad saved slot A view four fifth tile callback
2D14 02FB source aux A200 A000 parity"` found no indexed session. Reused
and independently checked the merged [view-four placement
walk](SLOT_A_U3BD_VIEW4_PLACEMENT.md), [group-zero byte-loop
model](SLOT_A_U40C_FIRST_TILE_PLANE.md), [fourth-callback
capture](SLOT_A_U3BD_VIEW4_TILE04.md), and later [group-three
tile](SLOT_A_U40C_GROUP3_TILE_PLANE.md) and [metadata
audit](SLOT_A_U40C_GROUP3_DIRTY_METADATA.md) as route and ABI leads. This
fifth call has a separate fresh native pre/post capture.

## Pinned original route

Ignored `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The local caller `+0x0211..+0x031F`, file `[0x2EB08,0x2EC16)`, has 270
bytes, 108 aligned 16-bit instructions, SHA-256
`b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03`.
It executes `LCALL 0x428:002A` at `+0x02FB`, returning at `+0x0300`.
The entry-2 dispatcher file `[0x32392,0x324E2)` has 336 bytes, SHA-256
`2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00`.
Its `+0x0166..+0x016E` range accepts item 13 in group zero,
`+0x020F..+0x0213` subtracts the group-zero remap, and the nonzero
flag at `+0x0216` selects `LCALL 0x464:08D4` at `+0x0249`.

Ignored `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
`RES_0464:08D4..0D71` maps to file `[0x4F4D,0x53EA)`, 1,181 bytes,
SHA-256 `9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe`.
Its auxiliary-present byte loop is file `[0x5243,0x5338)`, 245 bytes,
SHA-256 `fa187400915013d150d014e7200c4a45104c9f389b02859479281f8fefb492b2`.
At `+0x08E6..+0x08EE`, the resident checks both auxiliary far-pointer
words. At `+0x0C58..+0x0C6A`, it applies `(old destination AND auxiliary
row mask) OR source plane byte` and stores at destination data field
`+0x17`.

The fresh sandboxed replay passed four exact `2D14:02FB/0300` pre/post
pairs and stopped at the fifth pre-call `2D14:02FB`, then the fifth
post-call `2D14:0300`. At `SS:SP=1D16:3E78`, the two flag words are 1,
item is 13 at `SP+4`, y is 5 at `SP+6`, and x is 4 at `SP+8`.
`DS:[0x2722]=1` remaps the item to index 12; video state
`DS:[0x4A1E]=1` selects the masked route. The source far pointer
`DS:[0x5E3A]` resolves to physical `0x417B0`; its header auxiliary
pointer `+0x13/+0x15` resolves to `0x427C8`. The destination far
pointer `DS:[0x4A20]` resolves to `0x3DE48`. Source dimensions are
8 rows by 1 column, item stride 32 bytes; destination dimensions are
168 rows by 21 columns. Destination data begins after a 23-byte header;
this tile starts at data offset `((5*8)*21+4)*4 = 3376`.

## Immediate native effect and C comparison

The ignored private captures are under
`resources/oracle-runs/slot-a-view4-call05-p5-002/`:

| Stop | Full 640 KiB RAM SHA-256 | 14,135-byte destination SHA-256 |
| --- | --- | --- |
| Fifth pre-call `2D14:02FB` | `745d2edc6ea710a3b33f21e146f61a0711be55ca791c43f044c027f26ad5b619` | `92075358a41fa616b54e7fb3bcfd6d278cd36e58a4f5ff00b042dec3cf27f485` |
| Fifth post-call `2D14:0300` | `64010a82454fa0d467fe2f78e95e1a089f28a6ea6ac90b3c0ef408fdb80ec033` | `0f71e95decf6de154ce3d67a869386791b0c859a82a0cd7fd89aadb3aa9b8f9b` |

The resident changes 16 destination bytes at object offsets
`0x0D49..0x0F96`: plane-byte positions 2 and 3 in each of eight rows.
The first changed data offset is 3378. A private probe passed the live
pre-call source, auxiliary mask, destination, remap, video state and tuple
to merged `pr_u40c_entry2_mode1_tile_plane`. It returned `OK`, index 12,
first data offset 3376, 32 writes and 16 changed bytes. Its complete
14,135-byte output equals the native post-call object. The full-RAM diff
has 29 changed bytes: these 16 object bytes and 13 transient stack bytes.

Four EGA read-map planes were dumped at each stop for the first 8,192
bytes of both `A200` and `A000`. All 65,536 sampled bytes match
pre/post. Concatenated four-plane SHA-256 is unchanged: A200
`c35020473aed1b4642cd726cad727b63fff2824ad68cedd7ffb73c7cbd890479`;
A000 `8fa4f755484abda61c75dbf6aba7b7dadca4f70906b368a7c2bdc8c7c74dadaf`.
This establishes the immediate sampled-window result only; a later flush
or pixels elsewhere in the aperture are outside this capture.

The synthetic test uses item 13 at `(4,5)`, group-zero remap 1, video
state 1, and matching source/destination shapes. Its artificial mask
clears one bit in all eight rows while source bytes restore it in two
planes. It checks 16 changed plane bytes, the entire surrounding object,
and rejection of an auxiliary span one byte too short before mutation.
No original art bytes appear in the test.

## Reproduction

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
cmake -S . -B resources/build-slot-a-view4-tile05-p5 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-view4-tile05-p5 \
  --target poolrad_slot_a_view4_tile05_tests
ctest --test-dir resources/build-slot-a-view4-tile05-p5 \
  -R '^poolrad_slot_a_view4_tile05_tests$' --output-on-failure
```

Original executables, saves, RAM, EGA planes, and private screenshots
remain under ignored `resources/`.
