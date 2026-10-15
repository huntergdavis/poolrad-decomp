# Saved slot-A U40C group-two tile effect

The saved first-view U3BD view-three placement reaches three group-two
unit-`0x40C` entry-2 calls: `(2,6,143)`, `(2,7,144)`, and `(2,8,143)`.
The bounded portable C model added here selects the group-two source
and reproduces the reached resident masked-store effect on one
14,135-byte offscreen object. Each 32-store call matches the complete
object at the next native boundary: 16, 18, and 24 net changed
bytes respectively. The destination is retained across all three calls;
no expected post-call object is assigned to it.

This model covers the reached group-two tile data path, not all of U40C
entry 2, resident `0464:08D4`, dirty metadata, or a visible EGA page.
The first-view image remains incomplete. No strict full-C or verified
game-identity count is claimed.

Before implementation, bounded
`deja "Poolrad U40C group two item 143 resident 0464 08D4 tile effect view three"`
timed out after 30 seconds without an indexed result. Reused and
independently byte-checked the merged [group-zero tile model](SLOT_A_U40C_FIRST_TILE_PLANE.md),
[group-three tile model](SLOT_A_U40C_GROUP3_TILE_PLANE.md), and the
[first unsupported boundary](SLOT_A_U3BD_VIEW2_GROUP2_BOUNDARY.md).
The older local 80-call composition and flush probes are context only;
they are not imported into this branch or counted as this run's page
evidence.

## Original dispatch and reached resident effect

Ignored DOS v1.3 `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
`START.EXE` is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The hash-locked original spans are:

| Role | Half-open original file span | SHA-256 |
| --- | --- | --- |
| U40C entry-2 dispatcher, 336 bytes | `GAME.OVR [0x32392,0x324E2)` | `2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00` |
| Resident tile callee, 1,181 bytes | `START.EXE [0x4F4D,0x53EA)` | `9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe` |
| Reached masked store, four bytes | `START.EXE [0x52E3,0x52E7)` | `f1798d82c73cbb1d10f18e8d3d5f89265a778c5e2ea9cdfaca48e0dd4d68ec90` |

U40C `+0186..+0194` selects item IDs `0x74..0xB9` as group 2.
`+01F9` selects source far pointer `DS:[0x5E3A+4*group]`;
`+020F` reads remap `DS:[0x2722+2*group]` and `+0213` subtracts it.
For the saved calls, `DS:[0x2726]=116`, so IDs 143 and 144 select
source indices 27 and 28. With nonzero flag, U40C `+0249` far-calls
`0464:08D4`, passing the selected source, destination, flag, index,
and low coordinate bytes. The saved video state is one.

The reached source header describes an 8×1 tile with 32 bytes per
item, an auxiliary far pointer at header `+0x13`, and caller-resolved
row masks. The destination header describes 168×21 cells. The
resident `+0x0BCA..+0x0CB9` path advances eight rows and four plane
bytes per row. At `+0x0C6A`, the destination byte receives
`(old & row_mask) | source_byte`. The C model checks the item class,
flag, video state, object shape, and every source/mask/destination
bound before writing any destination byte. Unsupported routes return
an explicit status.

## Same-run native/C object gate

The ignored `slot-a-art-next-p5-005` RAM images belong to the earlier
reviewed +097F view-three replay. The linked
[boundary note](SLOT_A_U3BD_VIEW2_GROUP2_BOUNDARY.md) identifies the
run's provenance limit: its RAM files and caller/argument words are
hash locked, but no debugger transcript was retained. The new audit
also hash-locks the fourth call's pre-state and the `+0AA9` exit. At
each group-two pre-state, `SS:SP=1D16:3E78`, parent return
`[BP+2]=0AA9`, flags `(1,1)`, the live source and auxiliary pointers,
and destination `DS:[4A20]=3DE4:0008` are checked. The source object
and mask are copied from that **pre-call** RAM into C inputs; the
post-object is used only for comparison.

| Call | Group-two index | Net changed bytes to next stop | C/native next full object SHA-256 |
| --- | ---: | ---: | --- |
| `(2,6,143)` | 27 | 16 | `fac5056ba7d23fa77f4301f1ec1e197fa94e45a464f02be28ffab607d41f959d` |
| `(2,7,144)` | 28 | 18 | `df087df9df271fc56398b5a14c1dc86413653f73c3e32cad84cae932fd364eb9` |
| `(2,8,143)` | 27 | 24 | `e2a8d70f54501d664f3d47e50e0b614d563f36dbc3cde6eb15de6210d1c0ce94` |

The C destination equals native at each captured pre-state and its
following captured stop, **14,135/14,135 bytes** per boundary. The
native run did not capture each immediate `+0300` return, so stores
of the same value or transient effects between stops are not excluded.
The C model records 96 plane-byte stores in total. The final
comparison is at U3BD `+0AA9`; this private run
has no later same-run stop. The next route toward view-four and
`RES_0464:1B52` page transfer therefore needs a new joined capture.
No staging plane, page, indexed pixel, or RGB parity follows from
these object comparisons.

Synthetic CTest varies source bits and row masks, repeats a write to
prove the prior C result is read, exercises a same-value 32-store
call, and checks class/flag/video/bounds rejection without partial
writes. All media and native RAM stay under ignored `resources/`.

## Reproduce

```sh
python3 tools/replay/u3bd_group2_tile_audit.py
cmake -S . -B resources/build-u40c-group2-tile \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u40c-group2-tile \
  --target poolrad_overlay_u40c_group2_tile_plane_tests
ctest --test-dir resources/build-u40c-group2-tile \
  -R '^poolrad_overlay_u40c_group2_tile_plane_tests$' \
  --output-on-failure
```
