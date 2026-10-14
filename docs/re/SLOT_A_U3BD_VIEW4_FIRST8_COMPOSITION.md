# Saved slot-A view-four: first eight tile callbacks composed

One native DOS run reached the view-four placement gate at `2D14:0AD5`
and then eight successive `2D14:02FB` tile-call entries and `2D14:0300`
returns. The existing bounded group-zero C tile-plane model, called eight
times in that order on **one persistent destination object**, equals the
entire 14,135-byte native object after every call. Five calls change the
object; the first, second, and fourth do not. This is an offscreen object
prefix, not the rest of the view-four placement, its display transfer, a
complete static frame, or a full-C routine claim.

Before debugging, `deja "Poolrad saved slot A view four first eight tile
callback composition native object hashes"` found no indexed session.
The merged [view-four placement walk](SLOT_A_U3BD_VIEW4_PLACEMENT.md),
[group-zero plane-byte model](SLOT_A_U40C_FIRST_TILE_PLANE.md), and
[individual eighth-call evidence](SLOT_A_U3BD_VIEW4_TILE08.md) supplied
the route and ABI leads. This audit checks all eight against a **single
run**, including the seven intermediate native-object joins.

## Code and capture boundary

Ignored `SAVGAMA.DAT` SHA-256 is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
Ignored `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The unit `0x3BD` local caller `+0x0211..+0x031F` maps to file
`[0x2EB08,0x2EC16)`, 270 bytes, SHA-256
`b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03`.
Each reached `+0x02FB` far call returns at `+0x0300`.
The unit `0x40C` entry-2 dispatcher maps to file
`[0x32392,0x324E2)`, 336 bytes, SHA-256
`2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00`.
It remaps group-zero IDs at `+0x020F..+0x0213` and selects resident
`0x464:08D4` at `+0x0249` for the nonzero flag.

Ignored `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Resident `0x464:08D4..0D71` maps to file `[0x4F4D,0x53EA)`,
1,181 bytes, SHA-256
`9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe`.
Its reached auxiliary-mask byte loop maps to `[0x5243,0x5338)`,
245 bytes, SHA-256
`fa187400915013d150d014e7200c4a45104c9f389b02859479281f8fefb492b2`.
The reached store at `+0x0C6A` uses `(old destination AND mask row) OR
source plane byte`.

At every pre-call stop, `SS:SP=1D16:3E78` supplies two one-valued flag
words, item at `SP+4`, y at `SP+6`, and x at `SP+8`.
`DS:[0x2722]=1` supplies the group-zero remap and `DS:[0x4A1E]=1`
selects the masked video path. The source far pointer at `DS:[0x5E3A]`
resolves to physical `0x417B0`; its header auxiliary pointer resolves to
`0x427C8`; the destination pointer at `DS:[0x4A20]` resolves to
`0x3DE48`. Their captured hashes are constant at all eight entries:
2,257-byte source
`2bb731ff8bc5c51941cb118e7bf369694469d035b15b80d79531710b9114781d`,
360-byte mask
`44989553e7fac76ea3aa5938a33b5ec38386abe25d1aff96336de312f0696d0d`.
The destination object has a 23-byte header and 168-by-21-by-4 plane
bytes.

## Ordered native/C object comparison

Private same-run 640 KiB pre/post RAM captures remain in ignored
`resources/oracle-runs/slot-a-view4-first8-compose-p5-001/`.
The first pre-call RAM SHA-256 is
`6564bb63824040690a3c513491724e66ef84bd231b86e6fd5d9d301b61d9bbc4`;
the eighth post-call RAM SHA-256 is
`9312b35c2ca19ca12bb3d5ebc2735ee274700852e020e4a77a688e8cfdf6e931`.
The seventh post-call destination equals the eighth pre-call destination,
as do all six earlier adjacent joins.

| Call | `(x,y,item)` | Remapped index | Native/C changed bytes | Native/C post-call destination SHA-256 |
| --- | --- | ---: | ---: | --- |
| Before first | — | — | — | `f54d5c29e66e399b7bf9ceae7685c1b83e2a00fd15cb55a7dd771a48a9c48e3d` |
| 1 | `(4,3,1)` | 0 | 0 | `f54d5c29e66e399b7bf9ceae7685c1b83e2a00fd15cb55a7dd771a48a9c48e3d` |
| 2 | `(5,3,1)` | 0 | 0 | `f54d5c29e66e399b7bf9ceae7685c1b83e2a00fd15cb55a7dd771a48a9c48e3d` |
| 3 | `(4,4,18)` | 17 | 14 | `92075358a41fa616b54e7fb3bcfd6d278cd36e58a4f5ff00b042dec3cf27f485` |
| 4 | `(5,4,1)` | 0 | 0 | `92075358a41fa616b54e7fb3bcfd6d278cd36e58a4f5ff00b042dec3cf27f485` |
| 5 | `(4,5,13)` | 12 | 16 | `0f71e95decf6de154ce3d67a869386791b0c859a82a0cd7fd89aadb3aa9b8f9b` |
| 6 | `(5,5,18)` | 17 | 14 | `19bb17c39c7b0070c2970b68ff736acf7d1b18549647b825223b8fa8c440424e` |
| 7 | `(4,6,13)` | 12 | 8 | `8375d239031484d4aebce9ba849de7b90cddae93ebe18d38a8f6ca2b201bd7ca` |
| 8 | `(5,6,13)` | 12 | 16 | `0c1d5eb4b3b1d9aebe59bfef530b213b5d21699c4f513108152a52dd1d8efbde` |

The private strict-warning probe initialized the C destination **once**
from native pre-first RAM. For each call it independently decoded the
stack tuple, remap, video state, and source/mask/destination far pointers
from that call's native pre-RAM, verified source/mask hashes, and passed
them to `pr_u40c_entry2_mode1_tile_plane`. It never assigned expected
native post-call bytes into the C destination. All eight calls returned
`OK`, reported 32 plane-byte writes each, and matched each complete
native post-call object byte for byte. Sixty-eight destination bytes
differ between native pre-first and post-eighth objects.

The public synthetic test uses the same eight call tuples and a single
caller-owned object. Distinct artificial source/mask patterns make the
three ID-1 calls no-ops, ID-18 calls change two planes, and ID-13 calls
change one plane. It checks cumulative whole-object contents after each
call, including previously changed regions and untouched bytes. No
original art, RAM, save, screen, or decoded pixels are public.

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
cmake -S . -B resources/build-slot-a-view4-first8-compose-p5 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-view4-first8-compose-p5 -j 8
ctest --test-dir resources/build-slot-a-view4-first8-compose-p5 \
  --output-on-failure
```
