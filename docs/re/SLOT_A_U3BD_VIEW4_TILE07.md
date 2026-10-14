# Saved slot-A view-four callback seven: one changed plane

The saved slot-A view-four placement reaches a seventh tile callback
at `2D14:02FB`, after the [sixth callback](SLOT_A_U3BD_VIEW4_TILE06.md)
returns at `+0x0300`. Its live tuple is
`(x=4,y=6,item=13,flag=1,other=1)`. The merged bounded group-zero
plane-byte model matches the **entire immediate native destination
object**. This is one offscreen tile effect, not a complete frame,
later display transfer, full-C translation, or verified identity count.

Before debugging, `deja "Poolrad saved slot A view four seventh tile
callback 2D14 02FB group zero parity"` found no indexed match.
The merged [view-four placement walk](SLOT_A_U3BD_VIEW4_PLACEMENT.md),
[group-zero byte-loop model](SLOT_A_U40C_FIRST_TILE_PLANE.md), and
[sixth-callback capture](SLOT_A_U3BD_VIEW4_TILE06.md) supplied the
route, ABI, and model leads. This seventh call has its own native
pre/post RAM and EGA capture.

## Pinned route and live input

Ignored `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The ignored slot-A `SAVGAMA.DAT` SHA-256 is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The local caller `+0x0211..+0x031F`, file
`[0x2EB08,0x2EC16)`, has 270 bytes, SHA-256
`b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03`.
Its `LCALL 0x428:002A` at `+0x02FB` returns at `+0x0300`.
The entry-2 dispatcher file `[0x32392,0x324E2)` has 336 bytes,
SHA-256 `2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00`.
It admits item 13 in group zero, subtracts the remap at
`+0x020F..+0x0213`, and the nonzero flag selects
`LCALL 0x464:08D4` at `+0x0249`.

Ignored `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
`RES_0464:08D4..0D71` maps to file `[0x4F4D,0x53EA)`, 1,181
bytes, SHA-256
`9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe`.
The auxiliary-present byte loop is file `[0x5243,0x5338)`,
245 bytes, SHA-256
`fa187400915013d150d014e7200c4a45104c9f389b02859479281f8fefb492b2`.
The resident checks both auxiliary pointer words at
`+0x08E6..+0x08EE`; its reached store at `+0x0C6A` applies
`(old destination AND auxiliary row mask) OR source plane byte`.

The sandboxed replay stopped at the view-four gate `2D14:0AD5`,
passed six exact `2D14:02FB/0300` pre/post pairs, and stopped at
the seventh pre-call `2D14:02FB` and return `2D14:0300`.
At `SS:SP=1D16:3E78`, both flag words are 1, item 13 is at
`SP+4`, y=6 at `SP+6`, and x=4 at `SP+8`. Return
`SP=3E82`.

`DS:[0x2722]=1` remaps item 13 to index 12;
`DS:[0x4A1E]=1` selects the masked route. Source far pointer
`DS:[0x5E3A]` resolves to physical `0x417B0`; its header
auxiliary pointer `+0x13/+0x15` resolves to `0x427C8`.
Destination far pointer `DS:[0x4A20]` resolves to `0x3DE48`.
Source dimensions are 8 rows by 1 column with a 32-byte item stride;
destination dimensions are 168 rows by 21 columns. The destination
has a 23-byte header. This tile begins at data offset
`((6*8)*21+4)*4 = 4048` (`0x0FD0`).
The private 2,257-byte source object hashes to
`2bb731ff8bc5c51941cb118e7bf369694469d035b15b80d79531710b9114781d`;
the 360-byte auxiliary mask hashes to
`44989553e7fac76ea3aa5938a33b5ec38386abe25d1aff96336de312f0696d0d`.

## Immediate native effect and C comparison

Private captures are under ignored
`resources/oracle-runs/slot-a-view4-call07-p5-001/`:

| Stop | Full 640 KiB RAM SHA-256 | 14,135-byte destination SHA-256 |
| --- | --- | --- |
| Seventh pre-call `2D14:02FB` | `b6d429778256ec9d7081317a48d061126f0e31911545c5bdd5f00899afd0f29b` | `19bb17c39c7b0070c2970b68ff736acf7d1b18549647b825223b8fa8c440424e` |
| Seventh return `2D14:0300` | `3132eb6ba7c9ff6cd4f415029259402b9f905f38d92cf43a771c628e3a52f60c` | `8375d239031484d4aebce9ba849de7b90cddae93ebe18d38a8f6ca2b201bd7ca` |

The resident changes eight destination bytes, plane 3 in each of
eight rows. Their object offsets run `0x0FEA..0x1236`, and the
first changed data offset is `0x0FD3`. The full-RAM diff has 21
changed bytes: eight in the destination object and 13 outside it.
The pre-call destination hash equals the sixth callback's post-call
destination hash, independently linking the two stops.

A private strict-warning probe passed the live source, auxiliary
mask, destination, remap, video state and tuple to merged
`pr_u40c_entry2_mode1_tile_plane`. It returned `OK`, index 12,
first data offset `0x0FD0`, 32 writes and eight changed bytes. Its
complete 14,135-byte output equals the native post-call destination,
SHA-256
`8375d239031484d4aebce9ba849de7b90cddae93ebe18d38a8f6ca2b201bd7ca`.

Four EGA read-map planes were dumped at each stop for the first
8,192 bytes of both A200 and A000. All 65,536 sampled bytes match
pre/post. The unchanged four-plane SHA-256 values are A200
`c35020473aed1b4642cd726cad727b63fff2824ad68cedd7ffb73c7cbd890479`
and A000
`8fa4f755484abda61c75dbf6aba7b7dadca4f70906b368a7c2bdc8c7c74dadaf`.
This measures the immediate sampled windows only.

The synthetic regression tests a distinct one-plane case: its
artificial row mask clears a bit in all four planes while source
bytes restore it in planes 0 through 2, leaving eight plane-3
changes at `(4,6)`. It checks the entire surrounding object and
rejects a one-byte-short auxiliary span before mutation. No
original art bytes appear in the test.

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
cmake -S . -B resources/build-slot-a-view4-tile07-p5 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-view4-tile07-p5 \
  --target poolrad_slot_a_view4_tile07_tests
ctest --test-dir resources/build-slot-a-view4-tile07-p5 \
  -R '^poolrad_slot_a_view4_tile07_tests$' --output-on-failure
```

Original executables, saves, RAM, EGA planes, and screenshots remain
under ignored `resources/`.
