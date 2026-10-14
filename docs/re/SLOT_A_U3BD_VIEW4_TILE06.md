# Saved slot-A view-four callback six: group-zero masked bytes

The saved slot-A view-four placement reaches a sixth tile callback at
`2D14:02FB`, after the [fifth callback](SLOT_A_U3BD_VIEW4_TILE05.md)
returns at `+0x0300`. The live stack tuple is
`(x=5,y=5,item=18,flag=1,other=1)`. The merged bounded group-zero
plane-byte model matches the **entire immediate native destination
object**. This captures one callback and its offscreen object effect;
it does not prove a complete frame, later display transfer, full-C
translation, or another verified game identity.

Before debugging, `deja "Poolrad saved slot A view four sixth tile callback
2D14 02FB auxiliary mask A200 A000 parity"` found no indexed match.
The merged [view-four placement walk](SLOT_A_U3BD_VIEW4_PLACEMENT.md),
[group-zero byte-loop model](SLOT_A_U40C_FIRST_TILE_PLANE.md), and
[fifth-callback capture](SLOT_A_U3BD_VIEW4_TILE05.md) supplied the
route, ABI, and model leads. This sixth call has a separate fresh
native pre/post capture.

## Original route and input

Ignored `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The local caller `+0x0211..+0x031F`, file
`[0x2EB08,0x2EC16)`, has 270 bytes, SHA-256
`b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03`.
It executes `LCALL 0x428:002A` at `+0x02FB` and returns at `+0x0300`.
The entry-2 dispatcher file `[0x32392,0x324E2)` has 336 bytes,
SHA-256 `2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00`.
Its group-zero range admits item 18; the remap at
`+0x020F..+0x0213` produces index 17. The nonzero flag selects
`LCALL 0x464:08D4` at `+0x0249`.

Ignored `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
`RES_0464:08D4..0D71` maps to file `[0x4F4D,0x53EA)`, 1,181
bytes, SHA-256
`9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe`.
Its auxiliary-present byte loop is file `[0x5243,0x5338)`,
245 bytes, SHA-256
`fa187400915013d150d014e7200c4a45104c9f389b02859479281f8fefb492b2`.
It checks both auxiliary pointer words at `+0x08E6..+0x08EE`;
the reached store at `+0x0C6A` applies
`(old destination AND auxiliary row mask) OR source plane byte`.

The sandboxed replay stopped at the view-four gate `2D14:0AD5`,
passed five exact `2D14:02FB/0300` pre/post pairs, and stopped
at the sixth pre-call `2D14:02FB` and post-call `2D14:0300`.
At `SS:SP=1D16:3E78`, the two flag words are 1, item 18 is at
`SP+4`, y=5 at `SP+6`, and x=5 at `SP+8`. The post-call
`SP=3E82`.

`DS:[0x2722]=1` remaps item 18 to index 17 and
`DS:[0x4A1E]=1` selects the masked route. The source far pointer
`DS:[0x5E3A]` resolves to physical `0x417B0`; its header
auxiliary pointer `+0x13/+0x15` resolves to `0x427C8`.
The destination far pointer `DS:[0x4A20]` resolves to `0x3DE48`.
Source dimensions are 8 rows by 1 column, item stride 32 bytes;
destination dimensions are 168 rows by 21 columns. Destination
data begins after a 23-byte header, and this tile starts at data
offset `((5*8)*21+5)*4 = 3380` (`0x0D34`).

## Native effect and bounded C comparison

Private captures are under ignored
`resources/oracle-runs/slot-a-view4-call06-p5-001/`:

| Stop | Full 640 KiB RAM SHA-256 | 14,135-byte destination SHA-256 |
| --- | --- | --- |
| Sixth pre-call `2D14:02FB` | `2aef53840f33d8108b9cca1422e7c286954f196861128b6ee55393afbfc50086` | `0f71e95decf6de154ce3d67a869386791b0c859a82a0cd7fd89aadb3aa9b8f9b` |
| Sixth post-call `2D14:0300` | `fecff4a859ef81b89662fd2d0dfc42a46c27cbfee7a48388545e2819948dffb0` | `19bb17c39c7b0070c2970b68ff736acf7d1b18549647b825223b8fa8c440424e` |

The resident changes 14 destination bytes at object offsets
`0x0DA1..0x0F9A`: planes 2 and 3 in seven rows. The eighth
row is unchanged. The first changed data offset is `0x0D8A`
(3386). The full-RAM diff has 23 changed bytes: these 14
destination bytes and nine transient stack bytes.

A private probe compiled the merged
`pr_u40c_entry2_mode1_tile_plane` model with strict C warnings and
passed it the live pre-call source, auxiliary mask, destination,
remap, video state, and tuple. It returned `OK`, index 17, first
data offset `0x0D34`, 32 writes and 14 changed bytes. Its complete
14,135-byte output equals the native post-call object, SHA-256
`19bb17c39c7b0070c2970b68ff736acf7d1b18549647b825223b8fa8c440424e`.

Four EGA read-map planes were dumped at each stop for the first 8,192
bytes of both A200 and A000. All 65,536 sampled bytes match
pre/post. The unchanged four-plane SHA-256 values are A200
`c35020473aed1b4642cd726cad727b63fff2824ad68cedd7ffb73c7cbd890479`
and A000
`8fa4f755484abda61c75dbf6aba7b7dadca4f70906b368a7c2bdc8c7c74dadaf`.
This measures the immediate sampled windows only.

The synthetic test uses item 18 at `(5,5)`, remap 1 and video
state 1. Its artificial mask and source bytes change two plane
bytes in seven rows while preserving the eighth. It checks the
complete surrounding object and rejects a one-byte-short auxiliary
span before mutation. No original art bytes appear in the test.

## Reproduction

```sh
python3 - <<'PY'
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
cmake -S . -B resources/build-slot-a-view4-tile06-p5 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-view4-tile06-p5 \
  --target poolrad_slot_a_view4_tile06_tests
ctest --test-dir resources/build-slot-a-view4-tile06-p5 \
  -R '^poolrad_slot_a_view4_tile06_tests$' --output-on-failure
```

Original executables, saves, RAM, EGA planes, and screenshots remain
under ignored `resources/`.
