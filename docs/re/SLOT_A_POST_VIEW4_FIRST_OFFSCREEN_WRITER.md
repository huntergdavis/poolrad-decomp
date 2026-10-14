# Saved slot-A: first value-changing offscreen call after view-four placement

The saved route reaches unit `0x3BD:+0x0AD8` **three times**. The first
two hits skip local placement through the zero gate; the third returns
from the 16-tile `+0x0211` view-four placement and has the audited
14,135-byte offscreen-object SHA-256
`8664637ddd0016f3ef9978c4694f87ecbb7fb325ab147475f936c410ce9c6e8d`.
The first later tile call, `(x,y,item)=(12,5,1)`, leaves that object
unchanged. The second, `(12,6,40)`, is the first **observed
value-changing completed tile call**: resident `0464:+0x0C6A` writes 16
changed object bytes over rows 48..55. At the subsequent display-flush
entry, all 16 positions still differ from the pre-call object, but later
writes have replaced eight of the item-40 values. The merged group-zero
tile-plane C model
matches this call's complete immediate native post-object,
**14,135/14,135 bytes**. The other 599 net offscreen-byte changes before
the flush remain outside this audit.

Before debugging, `deja "Poolrad saved slot A view four 615 byte offscreen
writer placement return 2D14:0AD8 flush entry 0C88:1B52"` found no
matching indexed session. Reused and checked the merged
[view-four placement gate](SLOT_A_U3BD_VIEW4_PLACEMENT.md),
[group-zero tile plane model](SLOT_A_U40C_FIRST_TILE_PLANE.md), and
[all-dirty transfer boundary](SLOT_A_U40C_ALL_DIRTY_TRANSFER_MODEL.md).
The merged PR #373 calls 11–16 composition supplied the post-sixteenth
object hash; this fresh native run independently rechecked it. No full
frame or strict full-C claim follows from this bounded writer trace.

## Pinned original and native route

The ignored DOS v1.3 inputs are `START.EXE` SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
`GAME.OVR` SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and saved slot-A `SAVGAMA.DAT` SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private heavy DOSBox-X SHA-256 is
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

| Original role | File span | Bound | SHA-256 |
| --- | --- | ---: | --- |
| Parent continuation through later call | `GAME.OVR [0x2F3CF,0x2F486)`, unit `0x3BD:+0x0AD8..+0x0B8F` | 183 bytes / 72 aligned instructions | `83a9ba52f8b8d5fcd438b4388d9bfd5c18e757fdf7bae1df503ae2cc4feead97` |
| Reached loop and call branch | `GAME.OVR [0x2F44E,0x2F486)`, `+0x0B57..+0x0B8F` | 56 bytes / 25 aligned instructions | `b9d490d22bd8e92d888601bebe26c52d4ac4e945eab1d9b757fb3ebfb978d83a` |
| Local tile walker | `GAME.OVR [0x2EB08,0x2EC16)`, `+0x0211..+0x031F` | 270 bytes | `b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03` |
| Unit `0x40C` entry-2 dispatcher | `GAME.OVR [0x32392,0x324E2)` | 336 bytes | `2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00` |
| Resident masked-store path | `START.EXE [0x52BE,0x52E7)`, `0464:+0x0C45..+0x0C6E` | 41 bytes / 15 aligned instructions | `c3c456d06b70229f099a78f57ba95eec544c486c32cd0e04b9d13c78627a3c86` |
| Exact store instruction | `START.EXE [0x52E3,0x52E7)`, `0464:+0x0C6A..+0x0C6E` | 4 bytes | `f1798d82c73cbb1d10f18e8d3d5f89265a778c5e2ea9cdfaca48e0dd4d68ec90` |

At the verified third `2D14:0AD8` placement-return stop, the offscreen
far pointer
`DS:[4A20]` resolves to `3DE4:0008` (physical `0x3DE48`). Continuing
to the next `2D14:02FB` entry gives flag words 1/1, item 1, y 5, x 12
at `SS:3E78..3E80`; its immediate `+0x0300` return changes zero
object bytes. The next `+0x02FB` entry gives 1/1, item 40, y 6, x 12.
At `SS:BP=1D16:3E90`, `[BP+2]=0B8F`, `[BP+4]=2D14` proves its parent
call at unit `0x3BD:+0x0B8C`; `[BP+6]=3` is the literal view argument
pushed at `+0x0B88`. The local helper's far call reaches unit `0x40C`
entry 2 with group-zero remap `DS:[2722]=1`, so the item index is 39.
The nonzero flag and video-state byte `DS:[4A1E]=1` select resident
`0464:08D4` and its auxiliary-mask store loop.

The private debugger armed a memory-change watch at `3DE4:1011` only
after verifying the third `+0x0AD8` object hash. It reported
`00→7F` and stopped at **`0C88:0C6E`**, immediately after
`0C88:0C6A` stored to `ES:[DI+0x17]`. Live `ES=3DE4`, `DI=0x0FFA`,
and `DX=0x007F` connect that instruction to the watched address.
The stopped stack retained the `2D14:0300` far return and
`2D14:0B8F` parent return. Entry 2 had already replaced the stack
item word with index 39 at this mid-call stop; the pre-call RAM retains
the original item 40. A memory-change watch identifies a value-changing
store; it cannot exclude stores that write the same value.

## Same-run boundaries and C parity

Private 640 KiB captures remain under ignored
`resources/oracle-runs/slot-a-view4-writer-order-p5-002/`, with a
separate write-watch stop under the ignored `...writer-byte-p5-003/`.
The first call's pre, post, and second call's pre objects are identical
to the third `+0x0AD8` object. Their full-RAM hashes are respectively
`f70133941a9737e887618286699ee94e8885505679392b486d0a7823dd75fc0f`,
`82faf7062da4a2c3e1abdd7846d3e64cad2e7aab9533c61ff2d423396f0a19b4`,
and
`391a759cf5e9083156af3e4114ff0b1913a446cdc7b89cd569c3c12846162e8d`.
The item-40 immediate post-call RAM SHA-256 is
`224fad0a97131d157756426bc2b1912ff961498dbb391bdee194bf13cdb3dc3b`;
its complete object SHA-256 is
`8fe8460abf1e3a7ea3a7e8a1a2a80f0cddda97107133a19bffc0c96745b79fa0`.
The direct mid-call write-stop RAM SHA-256 is
`0006197ae4c487c9b0a109f413ae7198c9dc41c38c665eeeecb90bccef251adc`.

The private C probe decoded the live group-zero source and auxiliary-mask
far pointers from the item-40 pre-call RAM, initialized one C destination
from that same native pre-call object, then invoked
`pr_u40c_entry2_mode1_tile_plane` once. It reported item index 39,
first data offset 4080, first changed offset 4082, 32 byte stores and
16 changed bytes, exactly matching the complete native post-call object.
It did not copy any expected post-call bytes into the C destination.

The private file `ram-third-0ad8.bin` in the writer-order run has object
SHA-256 `d6652ff7e295001957eff907a855f7629dca6ba5579aa8634bafd44667d5e806`;
its filename alone does not identify the completed placement return. The
verified return capture is `ram-current-return.bin`, object SHA-256
`8664637ddd0016f3ef9978c4694f87ecbb7fb325ab147475f936c410ce9c6e8d`,
which matches the first later tile pre-state and the independent placement
audit. The branch and byte counts below use that verified return capture.

At the same run's `0C88:1B52` flush entry, the object SHA-256 is
`489572698be6487b4d9c88b470f62b530e0bd98539c99b75444a76b3639b14e1`,
identical to the prior run. Compared with the third `+0x0AD8` object,
615 bytes differ. All 16 item-40 changed positions still differ from their
pre-call values, but only eight retain the exact item-40 byte value; the
other eight have a third value at flush. Comparing the item-40 post-object
directly with the flush object gives 607 different positions.
The remaining 599 net changes need further caller sequencing. The
public synthetic test uses the two observed call tuples, a persistent
artificial object, a high-index host bounds rejection, and a 16-byte
plane effect with whole-object checks. No original art, RAM, save,
screen, or pixel data is published.

## Reproduction

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
for name, base, lo, hi, expected in (
    ('GAME.OVR', 0x2E8F7, 0x0B57, 0x0B8F, 'b9d490d22bd8e92d888601bebe26c52d4ac4e945eab1d9b757fb3ebfb978d83a'),
    ('START.EXE', 0x4F4D - 0x08D4, 0x0C45, 0x0C6E, 'c3c456d06b70229f099a78f57ba95eec544c486c32cd0e04b9d13c78627a3c86'),
):
    data = (Path('resources/DOS/POOLRAD') / name).read_bytes()[base+lo:base+hi]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(data, lo))
    assert ins[-1].address + ins[-1].size == hi
    assert sha256(data).hexdigest() == expected
    print(name, hex(base+lo), hex(base+hi), len(data), len(ins), expected)
PY
cmake -S . -B resources/build-slot-a-view4-first-writer-p5 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-view4-first-writer-p5 -j 8
ctest --test-dir resources/build-slot-a-view4-first-writer-p5 --output-on-failure
```
