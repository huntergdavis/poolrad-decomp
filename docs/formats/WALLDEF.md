# DOS WALLDEF block spans

The C reader accepts one caller-owned, already unpacked `WALLDEF*.DAX` block.
Its descriptor borrows the bytes and returns offsets only. Keep the block alive
and unchanged while using it. No art, pixels, or tile maps are decoded.

## Byte contract

A block must be exactly 780, 1,560, or 2,340 bytes: one, two, or three
wallsets. Each wallset is five consecutive 156-byte slices. Within a slice,
the ten fixed view ranges below partition all 156 bytes. The offset, column,
and row arrays are **in the DOS executable**, not stored at the start of each
slice. They occur consecutively in the supplied `START.EXE` at file offsets
`0xCE5A`, `0xCE64`, and `0xCE6E`; the arrays match the attributed MIT reference.

| view index | slice-relative range, end exclusive | rows × columns |
| ---: | ---: | ---: |
| 0 | 0–2 | 2 × 1 |
| 1 | 2–6 | 4 × 1 |
| 2 | 6–10 | 4 × 1 |
| 3 | 10–22 | 4 × 3 |
| 4 | 22–38 | 8 × 2 |
| 5 | 38–54 | 8 × 2 |
| 6 | 54–110 | 8 × 7 |
| 7 | 110–132 | 11 × 2 |
| 8 | 132–154 | 11 × 2 |
| 9 | 154–156 | 2 × 1 |

`pr_walldef_open` rejects all other lengths rather than truncating them.
`pr_walldef_slice_at` and `pr_walldef_view_at` check wallset, slice, and view
indices before calculating offsets. Spans use block-relative `source_offset`
and exclusive `end_offset`; callers can index the borrowed data only within
those ranges. The view descriptor also supplies raw row and column counts.
There are no returned pointers into or beyond the data.

The earlier [DOS kit audit](../FABLE_AUDIT.md) identified the 780/156-byte
period and cited Gold Box Explorer. A `deja "WALLDEF"` recall returned that
prior audit and the planning discussion for this issue. The fixed arrays and
slice handling were adapted from [Gold Box Explorer's pinned `DaxWallDefFile.cs`](https://github.com/bsimser/Gold-Box-Explorer/blob/eac30aba/src/Common/Plugins/Dax/DaxWallDefFile.cs);
its [MIT notice](GoldBoxExplorer-MIT.txt) is retained. The reference declares
an eleventh offset byte but indexes only ten views; in the DOS executable,
the byte immediately after the ten offsets is the first column count. The
reader uses exactly ten offsets.

## Tests and local read-only audit

```sh
cmake -S . -B /tmp/poolrad-walldef-build -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-walldef-build
ctest --test-dir /tmp/poolrad-walldef-build --output-on-failure
/tmp/poolrad-walldef-build/poolrad_walldef_audit resources/DOS/POOLRAD/WALLDEF[1-8].DAX
```

Synthetic tests cover all ten view bounds in all slices of a three-wallset
block, exact final boundaries, malformed lengths, invalid indices, and an
invalidated descriptor. On 2026-10-10, all five CTest targets passed with
warnings as errors. The audit read all eight ignored DOS v1.3 WALLDEF
archives, fully decoded their DAX blocks, and checked that every view span
partitions its slice without gaps or overruns. It wrote no decoded content:

```text
archives=8 blocks=38 wallsets=42 slices=210 validated_view_offsets=2100 failures=0
```

The unpacked size distribution was 35 blocks of 780 bytes, two of 1,560,
and one of 2,340. None of the 210 slices begins with the ten-byte reference
offset array, consistent with the arrays residing in `START.EXE`. The target
DOS executable SHA-256 checksums were:

```text
START.EXE  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd
GAME.OVR  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614
```

## Open meanings

Slice and view indices are positional only. The fixed face names and order
proposed by an earlier kit are unverified for this DOS build. Tile IDs, their
art, and the perspective meaning of each view remain outside this reader.
The wallset IDs selected for a map come from its ECL `0x37` operands; map 18's
`1,3,5` selection does not establish a general GEO wall-nibble formula.
