# Saved slot-A +097F bridge: first unsupported group-two tile

The reviewed [ten-call first-view object join](SLOT_A_U3BD_FIRST_VIEW_OBJECT_CHAIN.md)
ends at U3BD `+097F` with a complete 14,135-byte C offscreen object.
The next native view-two scan leaves that object unchanged. Its first
view-three tile, `(x,y,item)=(2,5,1)`, uses the merged group-zero C tile
model and leaves the complete object unchanged. The next tile,
`(2,6,143)`, is **group two** and has 16 net changed object bytes by
the following native call boundary. The current portable tile models
cover groups zero and three. This audit stops before predicting that
second call's effect or a visible page.

This is a bounded object-value bridge and a typed missing C dependency.
It does not establish a C first-view frame, a complete U3BD entry-five
translation, or a strict/full game-function count.

Before this investigation, bounded
`deja "Poolrad U3BD 097F first view object to EGA frame transfer 3BD 0464 saved slot A"`
timed out after 35 seconds without an indexed result. Reused and
independently checked the merged
[view-three placement route](SLOT_A_U3BD_VIEW3_PLACEMENT.md),
[group-zero tile contract](SLOT_A_U40C_FIRST_TILE_PLANE.md), and
[first-view object join](SLOT_A_U3BD_FIRST_VIEW_OBJECT_CHAIN.md).
The older local composed-flush commits `f976c30`, `1145e7b`, and
`c8a9902` are useful downstream evidence and probes, but are absent
from this branch; none is counted as a same-run join here.

## Hash-locked native boundary

Ignored DOS v1.3 `GAME.OVR`, `START.EXE`, and `SAVGAMA.DAT` hashes are
respectively
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
and
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
Unit `0x3BD` begins at `GAME.OVR` file `0x2E8F7` and is loaded at
segment `2D14`. The original half-open spans used by the audit are:

| Role | File interval | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| View-two scan `+097F..+0A16` | `[0x2F276,0x2F30D)` | 151 | `cffa2359b10e558baf9e85c444ca0524bbf21d5f7430c0c6ea719e6a8d7ca522` |
| Depth-one setup `+0A19..+0A7A` | `[0x2F310,0x2F371)` | 97 | `273127afaa4fec0c268aaddf137e9a6d66b74817a1375713bb3884bbd6ab204b` |
| View-three gate `+0A7A..+0AA9` | `[0x2F371,0x2F3A0)` | 47 | `61f65a27f0a5bbfae08e76491d8c44c3d7d6a9f77598d37bca99c83b19054c1b` |
| Local tile walker `+0211..+031F` | `[0x2EB08,0x2EC16)` | 270 | `b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03` |
| U40C entry-2 tile dispatcher | `[0x32392,0x324E2)` | 336 | `2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00` |

The ignored run `slot-a-art-next-p5-005` contains six 640-KiB RAM
captures in execution order: `+097F`, `+0A16`, first `+02FB` tile
handoff, its immediate `+0300` return, second `+02FB` handoff, and
third `+02FB` handoff. Their complete RAM SHA-256 values are pinned in
[`u3bd_view2_group2_boundary_audit.py`](../../tools/replay/u3bd_view2_group2_boundary_audit.py).
The first two native stops have identical offscreen-object SHA-256
`15ef1f9670025967b98c18778f0e9cb0d5e6ca96b77b48e12e92a5ff57d9ffbc`.
The separately reviewed ten-call run ends with that same **object**
hash; the full machine states belong to different executions and are
not equated.
The earlier view-three run recorded RAM stop files but no debugger
transcript for this bridge. Its ordered stop provenance comes from the
reviewed view-three note and the saved parent/argument words checked
against each RAM image; this audit independently pins every file hash.

At both tile handoffs, `SS:SP=1D16:3E78`, `SS:BP=1D16:3E90`, and
`[BP+2]=0AA9` identify the view-three parent return. Five pushed
words are `(1,1,item,y,x)`. At the first call they are
`(1,1,1,5,2)`; at the second, `(1,1,143,6,2)`. Live DS remap
words at `+2722..+272A` are `1,46,116,186,256`; the interval
`116<=143<186` selects group two and index 27. This is a dispatch
classification, not a group-two effect model.
The hash-locked U40C entry-2 instructions at `+0186..+0194` compare
the item against `0x74..0xB9` and store group 2. Its `+020F` reads
`DS:[0x2722+2*group]`, `+0213` subtracts that remap from the item,
and reached `+0249` far-calls resident `0464:08D4` with the selected
source pointer and destination. The original route identifies the
callee and selection; the group-two C data effect remains unmodeled.

## C/native gate and stopping point

The audit seeds one C destination from the continuation run's own
`+097F` native object. It checks equality at `+0A16` and the first
tile handoff, then calls merged `pr_u40c_entry2_mode1_tile_plane` with
the first handoff's live source, auxiliary mask, words, and video state.
The C call reports 32 stores and zero changed bytes. Its complete
14,135-byte output equals both the immediate native first return and
the second tile's native pre-object. No expected post-object is copied
into the C destination.

The native object differs by 16 bytes between the second and third
tile handoffs. Those stops bound an effect of the second callback
interval; they do not by themselves identify its individual store
instruction. A group-two tile effect must be recovered and tested
before this C object can be carried to view-four placement or the
later `RES_0464:1B52` visible transfer. The prior 615-byte net
post-view-four change is another downstream dependency. No `A200`,
`A000`, indexed pixel, or RGB comparison is claimed by this bridge.

## Reproduce

With original DOS inputs and private RAM captures under ignored
`resources/`:

```sh
python3 tools/replay/u3bd_view2_group2_boundary_audit.py
cmake -S . -B resources/build-u3bd-view2-group2-boundary \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u3bd-view2-group2-boundary \
  --target poolrad_overlay_u3bd_first_view_object_chain_tests
ctest --test-dir resources/build-u3bd-view2-group2-boundary \
  -R '^poolrad_overlay_u3bd_first_view_object_chain_tests$' \
  --output-on-failure
```

The original executable, save, RAM captures, object bytes, EGA planes,
screenshots, and decoded pixels remain under ignored `resources/`.
