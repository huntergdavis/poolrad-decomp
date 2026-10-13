# Saved slot-A first front-wall tile handoff

This note recovers one reached path inside DOS v1.3 `GAME.OVR` unit `0x3BD`
entry 5. The saved first-view call samples a wall nibble at `(13,4)`, selects
the mode-zero view of piece slice 14, and reaches two calls to overlay
`0x428:0x002A` with item IDs 1 and 13 at tile-grid coordinates `(7,6)` and
`(7,7)`. The accompanying C model covers this **first, distance-two,
nonzero front-wall path** and the called local `+0x0211` mode-zero placement
walk. It does not translate entry 5, later scan iterations, the setup draw,
or the `0x428:0x002A` pixel effects. It adds no full-C or identity credit.

Before debugging, `deja "Poolrad slot A first original tile unit 0x3BD
entry5 saved state GEO view"` found no exact prior session. A broader
`deja "Poolrad first view"` surfaced earlier wall-render leads; its old
WALLDEF block-count claim was not used. Reused and independently checked:
the [first-view call](SLOT_A_NATIVE_FIRST_VIEW_AFTER_196F.md), the
[static-view boundary](SLOT_A_STATIC_VIEW_FIRST_REGION.md), the
[merged nibble lookup](U3BD_NIBBLE_ENTRY2.md), the
[piece loader](U3BD_PIECES_ENTRY6.md), and the
[WALLDEF layout](../formats/WALLDEF.md).

## Original and private evidence

All executables, archives, save data, RAM snapshots, screen captures, and
the local C parity probe remain under ignored `resources/`. The public
source contains no original tile bytes or pixels.

| Ignored input | SHA-256 |
| --- | --- |
| `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `SAVGAMA.DAT` | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| `GEO2.DAX` | `2035246014efdb26de3114f4737d52a935ad8834f79b4a8e4880da2922b6d595` |
| `WALLDEF2.DAX` | `bfbd4106d6207ea22d0e5862074ddef582ae24772656b9e47251f1b00933995d` |

Unit `0x3BD` starts at `GAME.OVR[0x2E8F7]`. Independent Capstone 5.0.7
decode on the pinned file gives these complete, aligned spans:

| Unit span | File start | Bytes / instructions | SHA-256 |
| --- | ---: | ---: | --- |
| `+0x0211..+0x031E`, local tile-placement helper | `0x2EB08` | 270 / 108 | `b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03` |
| `+0x0682..+0x06C7`, distance-two setup | `0x2EF79` | 70 / 24 | `7c46a5e4c75ef27081033026555fcd2f9e2b574a91b5182d87acfc48d054ab0e` |
| `+0x06C8..+0x0743`, first front-wall selection | `0x2EFBF` | 124 / 58 | `7828d3e185f19c7ee32abc602aa563c99e0982e135bda094a86a72b5cf1680f9` |

The full entry-5 code `+0x05FD..+0x0DB3` is 1,975 bytes with SHA-256
`f618ac7c5ada7bda627b378fc0d6072ac9a882ab0e3f81ae29cea289cb5055a0`.
The 84 bytes after it up to entry 6 include inline data and are not
treated as code. The saved caller is unit `0x3B4` entry 1
`+0x016D..+0x02A4`, 312 bytes, SHA-256
`75ae0ccb49cb7acffa24ef792a88a9b3b9b9b2f2beabb86966be749959204916`.

The ignored native replay hit the first-view call at `3582:3382`, then
unit `0x3BD` at loaded `2D14`. The saved window-0 word `+0x1CC` is 1,
so unit `0x3B4:+0x0183..+0x0193` takes the selector-1 path. Its
`+0x0210` test sees saved window-0 `+0x1F6` equal to zero, leaving
`DS:0x6AAC` unchanged. The pre-call RAM image measures that mode byte
as zero, so entry 5's `+0x0603` branch reaches the view scan after
`+0x061D`'s separate setup call. The caller passes the saved party
bytes `(15,4,6)` at `+0x021D..+0x0229`.

| Reached stop | Evidence from original instructions and ignored CPU/RAM |
| --- | --- |
| `2D14:06D5` | First `push cs; call +0x047B`; `SS:SP=1D16:3EC0` has top words `2D14, 6, 4, 13`, hence pushed `(x,y,selector)=(13,4,6)`. |
| `2D14:073F` | The nonzero-wall branch calls local `+0x0211`; `SS:SP=1D16:3EBE` has top words `2D14, 0, 14, 4, 5`, hence `(x,y,slice,view)=(5,4,14,0)`. `DS:0x6AAC` is still zero. |
| First `2D14:02FB` | `0x428:0x002A` call; `SS:SP=1D16:3EA2` has top words `1,1,1,6,7`: original push order `(x,y,item,1,1)=(7,6,1,1,1)`. |
| Second `2D14:02FB` | Same call site and stack address; top words `1,1,13,7,7`: `(x,y,item,1,1)=(7,7,13,1,1)`. |

The pinned full-RAM hashes are `5386ff4f616eee4c01398669cc10db3da8d0b968f096b580599e2ef20b112b86`
at `+3382`, then `9656edbd6c6edc45010674642548bfed3404e148e2e570380ca7d5a31fb7d785`
at `+06D5`, `5211fab46bc0f66afbc424b4c25d4d76de2c6bbd80306f28b2a4c28b777adaf4`
at `+073F`, and `c5e8ad39baef09e9c908ae3b6e2ba1b8a260bd448d5a8597d3c4ef90af8a7f6b` /
`3d2c40ae2cb725e08b46f578e2403c84d46ab3ab7c5ff87d41a911ada7fc7336`
at the two `+02FB` stops. The private heavy-debug executable has the
hash already recorded in the first-view note.

## Data provenance and selected indices

At `+3382`, live `DS:0x6AAD..+0x6AAF` is `(15,4,6)`, map ID
`DS:0x84DC` is 20, and facing-6 steps at `DS:0x273C` and
`DS:0x2745` are signed `(-1,0)`. Entry 5 multiplies the step by
its first distance 2, giving `(13,4)` as signed word locals. At
`+0x06DB/+0x06EB` the original passes only AL from each local to the
`+0x031F` grid leaf, which compares those bytes as signed values in
0..15. For example, a synthetic word coordinate 256 becomes byte zero and
takes the in-grid branch; word 17 stays out of grid. The C report retains
the original word coordinates and narrows only for this gate and lookup.
The merged entry-2 lookup reads
offset `0x100 + 16*4 + 13 = 0x14D` of far `[0x6A5C]` and returns
the low nibble 14. The 1,024 live bytes at that pointer exactly match
the decoded body of `GEO2.DAX` block 20; live-body SHA-256 is
`ad16708446ea94cff037db17fd342e222a9166a4eae413c8dba4da94336b39f8`.

The saved window-0 piece words identify `(id,slot)=(2,1),(4,2),(1,3)`;
the earlier [native piece replay](SLOT_A_NATIVE_1C08_PIECE_GATE.md)
directly observed the first pair passed to entry 6. All three IDs are
present as 780-byte blocks in `WALLDEF2.DAX`. At the view stop,
`DS:0x272C` is 45 and the slot remap deltas from `DS:0x2722..+0x2728`
are 0, 70, and 140. Re-decoding those three blocks and applying the
entry-6 rule matches **780/780 bytes in each** corresponding region
of live far `[0x6A58]`. Their live region SHA-256 values are
`44ce21e5756acb31909aa49bf65d8b4d2b880e7168f4d1709122bcec58c04b55`,
`ff9386219b63d40a3d564772351923a8c1c0bbf91f25015044fc2bcaaa9ea503`,
and `780a2b373e4417bbe40ee2da8a092b04bf5980100d0e58ca78b017c6f1daca41`.

The fixed view-zero table bytes read by `+0x0211` at
`DS:0x0C8C`, `+0x0C96`, and `+0x0CA0` give offset 0, one column,
and two rows. The call supplies origin `(5,4)` through
`DS:0x0CAA` and `DS:0x0CBE`. For slice 14, the two reached
piece-buffer indices are `14*156 - 156 = 2028` and 2029. Their
live item IDs are 1 and 13; both are positive, so `+0x02FB`
executes twice. The `+0x02C4..+0x02FA` instructions add two to
the tile-grid coordinates and push `(7,6)` then `(7,7)`, matching
the direct debugger frames. The meaning of these grid units inside
`0x428:0x002A` remains unmodelled.

## C boundary and verification

`pr_u3bd_entry5_first_wall` models the reached first-distance,
previous-wall-zero path and calls the merged `pr_u3bd_entry2`.
For a nonzero wall it walks the valid mode-zero local `+0x0211`
slice, clips source grid coordinates to 0..10, skips zero item bytes,
and calls a host hook for each reached `0x428:0x002A` handoff.
Original geometry and art tables are supplied by the caller; the
model reports the out-of-grid second-lookup branch, unsupported selector
residue, and bounds failures
explicitly. It does not claim how the graphics helper writes pixels.
The private strict C probe fed the captured `+3382` RAM image to the
new model and independently compared its callback tuples with both
native `+02FB` frames: **2/2 match**.

```sh
cmake -S . -B resources/build-u3bd-first-wall \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u3bd-first-wall \
  --target poolrad_overlay_u3bd_first_wall_tests
ctest --test-dir resources/build-u3bd-first-wall \
  -R '^poolrad_overlay_u3bd_first_wall_tests$' --output-on-failure
```

The synthetic test checks the saved-shaped numeric route with unrelated
synthetic tile bytes, the zero-wall branch and out-of-grid boundary, zero items,
clipping before piece reads, reached-only short-buffer and missing-hook
behavior, and an unsupported selector. The first-view setup call
`+0x061D`, later wall faces, any renderer writes, and all pixels beyond
the [empirical 128-pixel region](SLOT_A_STATIC_VIEW_FIRST_REGION.md)
remain outside this result. The empirical item IDs 40/41 and their
pixel positions have **no proven connection** to these two native
`0x428:0x002A` calls; they are not silently substituted for IDs 1/13.
