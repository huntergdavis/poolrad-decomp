# Saved slot-A view-four: last six tile calls and display boundary

The saved view-four placement reaches 16 tile callbacks. This audit
composes callbacks 11 through 16 on **one persistent offscreen object**
using the merged group-three and group-zero plane models. Every complete
14,135-byte C object equals the immediate native post-call object, and
the sixth equals the placement return. Call 16 is a measured no-op on
that object. The native object then changes by **615 bytes** before the
already modeled display flush. That intervening writer is the next
causal gate; the placement result alone is not the flush input.

Before debugging, `deja "Poolrad saved slot A view four after eighth tile
callback next group offscreen display transfer"` found no matching
indexed session. Reused the merged [placement route](SLOT_A_U3BD_VIEW4_PLACEMENT.md),
[first-eight composition](SLOT_A_U3BD_VIEW4_FIRST8_COMPOSITION.md),
[group-three tile model](SLOT_A_U40C_GROUP3_TILE_PLANE.md),
[group-three dirty metadata](SLOT_A_U40C_GROUP3_DIRTY_METADATA.md),
and [all-dirty transfer](SLOT_A_U40C_ALL_DIRTY_TRANSFER_MODEL.md).
The saved route reaches the placement, but this is an offscreen and
immediate-flush comparison, not a complete C frame or full-routine claim.

## Original code and native stops

The ignored saved input has SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
Ignored `GAME.OVR` has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
`START.EXE` has SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

| Role | Original address / file span | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| Unit `0x3BD` placement caller | `+0x0211..+0x031F`, `GAME.OVR [0x2EB08,0x2EC16)` | 270 | `b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03` |
| Unit `0x40C` entry-2 dispatcher | `GAME.OVR [0x32392,0x324E2)` | 336 | `2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00` |
| Resident tile callee | `0464:08D4..0D71`, `START.EXE [0x4F4D,0x53EA)` | 1,181 | `9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe` |
| Reached masked-byte loop | `START.EXE [0x5243,0x5338)` | 245 | `fa187400915013d150d014e7200c4a45104c9f389b02859479281f8fefb492b2` |
| Resident flush | `0464:1B52..1E5B`, `START.EXE [0x61CB,0x64D5)` | 778 | `7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c` |

The native run stopped at placement `2D14:0AD5`, each tile call
`2D14:02FB` and return `2D14:0300`, placement return `2D14:0AD8`,
flush entry `0C88:1B52`, and immediate flush return `2D14:05FB`.
At each tile call, `SS:SP=1D16:3E78` supplies two flag words equal to
one, then item, y, x. `DS=14A0`, video state `DS:[4A1E]=1`, and
`DS:[4A20]` resolves to offscreen physical `0x3DE48`. Groups zero and
three use remap words 1 and 186 respectively. The original masked store
at resident `+0x0C6A` applies `(old & mask[row]) | source byte`.

## Ordered object parity

The private pre-eleventh object SHA-256 is
`bd575b292819c0a25b4151c250357952ec331186c6f46fb755b999ba52243fbc`.
It equals the same run's post-tenth object. The earlier
[first-eight audit](SLOT_A_U3BD_VIEW4_FIRST8_COMPOSITION.md) and
[ninth-call audit](SLOT_A_U40C_GROUP3_TILE_PLANE.md) establish the
preceding bounded models. This six-call probe starts at the native
post-tenth object; it does not independently compose calls 1 through 10.

| Call | `(x,y,item)` | Group / remap / index | Changed bytes | Native and C post-object SHA-256 |
| --- | --- | --- | ---: | --- |
| 11 | `(4,8,228)` | 3 / 186 / 42 | 32 | `1908d476df9965393c76c8cace4aa199d9a23fa9eb0c5a5fb0189cf8c399ff07` |
| 12 | `(5,8,236)` | 3 / 186 / 50 | 32 | `ead89a13d47f3bf5c3a59f70434f64d9ec49cb7264740249d72843ce551a4946` |
| 13 | `(4,9,228)` | 3 / 186 / 42 | 24 | `1a5435b74a404fe73146e899337aa1c440315c76e0a05703f687d7f060c701f9` |
| 14 | `(5,9,23)` | 0 / 1 / 22 | 27 | `7260f150e14e7077271694fb6c4d0af06eeecde12a67f2d3bfced548c7e60c26` |
| 15 | `(4,10,21)` | 0 / 1 / 20 | 17 | `8664637ddd0016f3ef9978c4694f87ecbb7fb325ab147475f936c410ce9c6e8d` |
| 16 | `(5,10,3)` | 0 / 1 / 2 | 0 | `8664637ddd0016f3ef9978c4694f87ecbb7fb325ab147475f936c410ce9c6e8d` |

The private strict-warning probe initializes the C object **once** from
native post-tenth RAM. At every later pre-call stop it decodes the live
stack tuple, group remap, source and auxiliary-mask far pointers, and
destination pointer. It checks that the C object equals native pre-call
RAM, calls the appropriate merged C model, then compares all 14,135
bytes with native post-call RAM. It never assigns a native expected
post-call object to the C buffer. All six models report 32 plane-byte
writes, and native/C changed-byte counts agree. Adjacent native object
joins and the placement return agree byte for byte.

## Measured boundary after placement

The placement-return object SHA-256 is
`8664637ddd0016f3ef9978c4694f87ecbb7fb325ab147475f936c410ce9c6e8d`.
In ignored run `slot-a-view4-writer-order-p5-002`, this post-sixteenth
`+0AD8` baseline is `ram-next-0ad8.bin` (also present in
`ram-after16-next-stop.bin` and `ram-after16-first-tile-pre.bin`);
the older `ram-third-0ad8.bin` has object SHA-256
`d6652ff7e295001957eff907a855f7629dca6ba5579aa8634bafd44667d5e806`
and is not this baseline.
The same run's flush-entry object SHA-256 is
`489572698be6487b4d9c88b470f62b530e0bd98539c99b75444a76b3639b14e1`.
The private 640 KiB RAM captures at those stops have SHA-256
`e05bb3cc8974b4bef870c7fbaf46177b09b9531f16b92eb19f4c7f6c0b4e2483`
and
`48becc9f9848794f232e6887a08edfd8b59d901833ccffbbbea4c336245080d7`.
There are **615 changed offscreen bytes**, zero header bytes: scanlines
17..103, in planes 0/2/3 (174/219/222 bytes; plane 1 has none).
These bytes change between `2D14:0AD8` and `0C88:1B52`. The writer
address and exact call path within that interval remain unproved. This
measured gap blocks a direct C placement-to-display prediction.

Starting instead from the **native flush-entry** object and same-run
pre-flush `A200`/`A000` plane windows, the merged all-dirty C transfer
matches all **27,928/27,928** immediate native post-flush plane bytes
over `[0x03C3,0x1166)`, and all **168/168** dirty-row metadata records.
The native flush changes 5,570 bytes in those captured plane windows.
The model does not supply the 615 missing offscreen bytes.

The public synthetic test composes the same six group and call tuples
against one caller-owned object. Distinct artificial source/mask patterns
exercise different plane effects, repeated group-three index 42, and
the final no-op. It checks whole-object state after every call, including
effects retained from earlier calls. Original art, save, RAM and EGA
plane captures stay in ignored `resources/`.

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
    ('START.EXE', 0x61CB, 0x64D5, '7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c'),
):
    data = (Path('resources/DOS/POOLRAD') / name).read_bytes()[lo:hi]
    assert sha256(data).hexdigest() == expected
    print(name, hex(lo), hex(hi), len(data), expected)
PY
cmake -S . -B resources/build-slot-a-view4-last6-p5 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-view4-last6-p5 -j 8
ctest --test-dir resources/build-slot-a-view4-last6-p5 --output-on-failure
python3 resources/oracle-runs/slot-a-view4-last6-compose-p5-001/last6_probe.py
python3 resources/oracle-runs/slot-a-view4-last6-compose-p5-001/flush_compare.py
```
