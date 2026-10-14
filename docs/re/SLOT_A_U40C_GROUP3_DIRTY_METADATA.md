# Saved view-four group-three tile: dirty-row and extent metadata

The tenth reached tile callback in saved slot A carries
`(x=5,y=7,item=0xE3,flag=1,other=1)`. Its offscreen plane-byte effect is
already explained by the merged [ninth-call group-three model](SLOT_A_U40C_GROUP3_TILE_PLANE.md)
(PR #346): a private probe also matched the tenth immediate post-call
object at **14,135/14,135 bytes**, with 24 changed bytes and 32 byte
writes. This follow-up models the distinct `RES_0464:08D4` dirty-row and
extent stores interleaved with that plane loop. It is a bounded model,
not a full resident routine, display transfer, or strict full-C claim.

Before debugging, `deja "Poolrad slot A view four tenth tile group three
RES_0464 08D4 dirty row extent metadata"` and the focused `deja "Poolrad
RES_0464 08D4 dirty row extent metadata 4A28 4AD0 4C20 4D70"`
found no exact indexed session. Reused and independently checked the
[first tile-plane note](SLOT_A_U40C_FIRST_TILE_PLANE.md) for the resident
frame and metadata-array roles, the [view-four caller](SLOT_A_U3BD_VIEW4_PLACEMENT.md)
for the placement route, and PR #346's group-three selector and
auxiliary-present loop evidence. The [all-dirty transfer model](SLOT_A_U40C_ALL_DIRTY_TRANSFER_MODEL.md)
uses these extents at the later video flush.

## Original branch and effects

Ignored `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
`RES_0464:08D4..0D71` occupies file `[0x4F4D,0x53EA)`:
1,181 bytes / 429 aligned 16-bit instructions, SHA-256
`9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe`.
Its auxiliary-present video-state-1 loop `+0x0BCA..+0x0CBF` occupies
`[0x5243,0x5338)`: 245 bytes / 86 instructions, SHA-256
`fa187400915013d150d014e7200c4a45104c9f389b02859479281f8fefb492b2`.
The two metadata subspans are:

| Resident offset | File span | Bytes / instructions | SHA-256 |
| --- | --- | ---: | --- |
| `+0x0BEB..+0x0C1F` | `[0x5264,0x5298)` | 52 / 17 | `206fee3b8a0efc6c627adaab238c67bbfed2643954241f29b7105019ce09bbcc` |
| `+0x0C8C..+0x0CB9` | `[0x5305,0x5332)` | 45 / 17 | `aefc9e22b9a44d46f4be5f0f57dc11315066134007140ecceb924637758db8dd` |

`+0x08DA..+0x08F6` tests flag bit zero and the complete source auxiliary
far pointer. The live pointer has zero offset but nonzero segment, so
`+0x0BCA` selects the masked branch. The live source is 8 rows by one
byte-column; the destination is 168 rows by 21 byte-columns. For each
source row `r=0..7`, the reached code uses
`row=y*8+r`, `first=((row*21)+x)*4`, and `last=first+3`.
It writes `DS:[0x4A28+row]=1`. If the existing first extent at
`DS:[0x4C20+2*row]` is greater than `first`, it writes the x word at
`DS:[0x4AD0+2*row]` and replaces that first extent. After four
plane-byte iterations, it raises `DS:[0x4D70+2*row]` only if
`last` is greater. This is the exact unsigned comparison direction
at `+0x0C02` and `+0x0C95`.

The C function accepts the measured shapes, video state one, flag bit
zero set, and a present auxiliary pointer. It checks caller buffer spans
before writing any row. It models only the four metadata arrays. The
original also writes the destination object and loop counter
`DS:[0x70C7]`; those and other video modes remain separate.

## Immediate native and C comparison

A fresh, hash-verified, network-isolated saved-slot-A replay stopped
before the tenth call at `2D14:02FB` and immediately after return at
`2D14:0300`. Private full-RAM SHA-256 values are
`5d295e9c449fe9ce1aa1955f1d6bca7433c87f57169b120086111871c4fded62`
and
`2a46d496ee5d118e611cd16558ca49f1078fb39882cf7fb7edf55a29fffe7264`.
All eight reached row flags and extents were already broad enough;
the call changes **zero** metadata bytes. The C model matches the
entire captured `DS:[0,0x4EC0)` prefix, **20,160/20,160 bytes**,
including its untouched gaps.

To exercise the update branches directly, a second private replay
stopped at the same native call and changed only row 56's dirty flag,
low word, first word and last word in **guest RAM**. Original files
and every other guest byte remained unchanged. The private debugger's
[documented `SM` command](https://github.com/joncampbell123/dosbox-x/blob/master/README.debugger)
performed those writes. The untouched,
controlled pre-call, and immediate native post-call full-RAM hashes
are respectively:

| Stop | SHA-256 |
| --- | --- |
| Untouched `2D14:02FB` | `bff9b0b3bdababc97c931b9f1a4543b060f11964a5473d665f6004aa891ce235` |
| Seven-byte row reset, before call | `5aef3cd87039fd801555cce1838d2b694692d3ba977ad12df66bb29774c35dd8` |
| Immediate `2D14:0300` | `c625d319234a061c34bdea9e31935aaef1964c682595ef62c56e4f61646e1a67` |

The original then changes exactly seven metadata bytes in that row:
`DS:4A60`, `4B40..4B41`, `4C90..4C91`, and
`4DE0..4DE1`. The C model predicts **all 20,160 bytes** of the
immediate native DS prefix and reports eight rows marked, one first
extent update, one last extent update, and seven changed bytes.
This controlled input proves the update decisions, while the
unmodified saved replay proves reachability and the saturated case.
It does not establish later display-page parity for this tile.

## Reproduction

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
b = Path('resources/DOS/POOLRAD/START.EXE').read_bytes()
assert sha256(b).hexdigest() == 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'
base = 0x4F4D - 0x08D4
for lo, hi in ((0x08D4, 0x0D71), (0x0BCA, 0x0CBF),
               (0x0BEB, 0x0C1F), (0x0C8C, 0x0CB9)):
    span = b[base+lo:base+hi]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, lo))
    assert ins[-1].address + ins[-1].size == hi
    print(hex(base+lo), len(span), len(ins), sha256(span).hexdigest())
PY
cmake -S . -B resources/build-slot-a-group3-dirty-p5 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-group3-dirty-p5 \
  --target poolrad_resident_464_group3_dirty_metadata_tests \
           poolrad_slot_a_u40c_group3_dirty_probe
ctest --test-dir resources/build-slot-a-group3-dirty-p5 \
  -R '^poolrad_resident_464_group3_dirty_metadata_tests$' --output-on-failure
resources/build-slot-a-group3-dirty-p5/poolrad_slot_a_u40c_group3_dirty_probe \
  resources/oracle-runs/slot-a-later-tile-p5-009/ram-call-10.bin \
  resources/oracle-runs/slot-a-later-tile-p5-009/ram-post-10.bin
resources/build-slot-a-group3-dirty-p5/poolrad_slot_a_u40c_group3_dirty_probe \
  resources/oracle-runs/slot-a-group3-dirty-mutate-p5-001/ram-call10-mutated.bin \
  resources/oracle-runs/slot-a-group3-dirty-mutate-p5-001/ram-post10-mutated.bin
```

The synthetic tests exercise the first and last extent branches,
overlapping tiles, the saved saturated case, idempotence, flag/pointer
gates, and host bounds. Original game media, RAM dumps, probe inputs,
and images remain under ignored `resources/`.
