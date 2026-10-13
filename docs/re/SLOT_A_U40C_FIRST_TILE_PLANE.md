# Saved slot-A first tile: reached offscreen plane-byte write

The saved first-wall route calls `GAME.OVR` unit `0x40C` entry 2 as
`0x428:002A` twice. This note identifies the first **changed byte in its
offscreen destination object** and models the reached byte loop in portable C.
It does not establish the eventual display transfer or translate the complete
overlay or resident routine. No full-C or game-identity credit is claimed.

Before debugging, `deja "Poolrad unit 0x40C entry 2 +015D +02AC overlay
0x428:002A first wall draw pixel effects"` found no exact prior session.
The broader `deja "Poolrad 0x40c draw"` recalled a font-loader lead. Reused
and independently checked: the [first-wall tile handoff](SLOT_A_U3BD_FIRST_WALL_TILE.md),
the [resident block wrapper](RESIDENT_464_BLOCK_WRAPPER.md), the
[font-loader route](FONT_8X8D_201.md), and the
[static view boundary](SLOT_A_STATIC_VIEW_FIRST_REGION.md). Their later
display behavior is not inferred from this offscreen write.

## Pinned original spans and reached route

Ignored `GAME.OVR` has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x40C` starts at file `0x32235`; entry 2 `+0x015D..+0x02AC`
occupies file `[0x32392,0x324E2)`, 336 bytes, SHA-256
`2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00`.
Independent Capstone 5.0.7 decode found 127 aligned instructions, eight
calls and 22 conditional/unconditional branches, ending in `RETF 0x0A`.
The saved caller at unit `0x3BD:+0x02FB` pushes `(x,y,item,1,1)` as
`(7,6,1,1,1)` then `(7,7,13,1,1)`; both original stack frames were captured
in the [handoff note](SLOT_A_U3BD_FIRST_WALL_TILE.md).

Entry 2 `+0x0163..+0x0174` selects group zero for both item IDs. The
far pointer at `DS:[0x5E3A]` is present at `+0x01F9..+0x0203`; the remap
word at `DS:[0x2722]` is 1, so `+0x0213` changes the IDs to indices 0 and
12. The nonzero `[BP+8]` gate at `+0x0216..+0x021A` takes the far call
at `+0x0249` to `RES_0464:08D4`. The separate `+0x0250` copy branch is
not reached by these frames.

Ignored `START.EXE` has SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
`RES_0464:08D4` maps to loaded image `0x04F14` and file
`[0x04F4D,0x053EA)`, ending immediately before the next prologue
`RES_0464:0D71`. Its 1,181 bytes have SHA-256
`9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe`;
the linear decode found 429 aligned instructions and `RETF 0x10` at
`+0x0D6E`. The far-call frame gives destination `[BP+6]`, source
`[BP+0x0A]`, flag `[BP+0x0E]`, item index `[BP+0x10]`, y `[BP+0x12]`,
and x `[BP+0x14]`.

Both native calls have video-state byte `DS:[0x4A1E]=1` and take the
`+0x0954 -> +0x0B74 -> +0x0B7B` path. The source object's auxiliary
pointer at `+0x13` is nonnull, selecting `+0x0BD3`. Its header measures
8 rows, one column and 32 bytes per item; the destination header measures
168 rows and 21 columns. At `+0x0C3D..+0x0C6A`, each of four plane bytes
per row receives `(old_destination & auxiliary_mask[row]) |
source[item_index, row, plane]`. The original also writes `DS:[0x4A28]`
dirty-row flags and `DS:[0x4AD0]`, `[0x4C20]`, `[0x4D70]` extent metadata
around this loop. Those metadata writes are outside the C model.

At the live call, `DS:[0x5376]` selects archive number 2 in the
unit-`0x40C` entry-1 name construction, whose prefix is `8X8D`.
Ignored `8X8D2.DAX` has SHA-256
`418fc6cfa448646d3f0b590b53d1c926a0b308a521260469c0d06d05a7869cf4`.
Its decoded block 1 is 2,257 bytes, SHA-256
`ab2f43f6825c7554289855990e64bd27c17c80337b7d86a95f27e0a356ed8bcf`.
The live constructed source object has 2,257 bytes, SHA-256
`2bb731ff8bc5c51941cb118e7bf369694469d035b15b80d79531710b9114781d`.
Only 92/2,257 bytes match the raw decoded block at identical offsets.
The constructor/postprocessor transformation has not been recovered here;
the model consumes the **live object shape**, not raw archive bytes.

## Private native comparison and C boundary

The ignored private replay stopped before and after each `0x428:002A`
call, recording full 640 KiB RAM images. The pre/post SHA-256 pairs are:

| Call | Pre | Post |
| --- | --- | --- |
| Item 1 at `(7,6)` | `215b83bac17a4bb4e8df503e5ada2bf460d7210a0d28812a9c735c97fd5a5a88` | `bdf5060fb5ea40194a47105c6de31a0ce6f2ac523a307b3473b09954ba9529f3` |
| Item 13 at `(7,7)` | `f8e0944688348b256965ef1732738b767282bf2b4b27f8413a4913119e706008` | `a57c3e7fa9a8dec84829cc289e66ac54b1637d9d531ad6879e7cc73efae44d7b` |

The live destination object begins at physical `0x3DE48`; its data starts
after the 23-byte header and spans 14,112 bytes. With tile-grid `(x,y)`,
the first selected data offset is `((y*8)*21+x)*4`; each later row advances
84 bytes. Thus the first and second calls start at data offsets 4,060 and
4,732. Both execute 32 plane-byte writes. Item 1 changes zero bytes;
item 13 changes nine, with the first change at data offset 4,734.
The private C parity probe fed each pre-call RAM object into
`pr_u40c_entry2_mode1_tile_plane` and compared its entire destination data
with the corresponding native post-call RAM: **14,112/14,112 bytes match
for each call**. This identifies an offscreen object mutation, not a
displayed pixel or a complete renderer effect.

The C function accepts group-zero IDs 1..45, nonzero reached flag 1,
video state 1, the measured source/destination shapes and a separate
auxiliary-mask span. Host bounds are checked before mutation. Other groups,
video modes, the `+0x0250` branch, DS extent metadata, and later display
transfer are explicit unsupported boundaries. Synthetic tests cover the
unchanged first item, changed plane bytes, item remap, branch gates and
buffer bounds without using any original art bytes.

## Reproduction

```sh
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
for path, start, end in (
    ('resources/DOS/POOLRAD/GAME.OVR', 0x32392, 0x324E2),
    ('resources/DOS/POOLRAD/START.EXE', 0x04F4D, 0x053EA),
):
    data = Path(path).read_bytes()[start:end]
    print(path, hex(start), hex(end), len(data), sha256(data).hexdigest())
PY
cmake -S . -B resources/build-u40c-tile-plane \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u40c-tile-plane \
  --target poolrad_overlay_u40c_tile_plane_tests
ctest --test-dir resources/build-u40c-tile-plane \
  -R '^poolrad_overlay_u40c_tile_plane_tests$' --output-on-failure
```

All executables, archives, saves, RAM snapshots, art bytes and screen
captures remain under ignored `resources/`. No original media or derived
pixel array is in this public change.
