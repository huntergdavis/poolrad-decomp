# RES_0464:02A5 video-state-2 route

This is a **bounded C model of one route** inside the merged
[postprocessor map](RESIDENT_464_POSTPROCESS_MAP.md), not a full translation
of `RES_0464:02A5`. Entry assumes its earlier nonzero-object and raw
video-byte-equals-2 gates have passed. It does not prove live LOAD MONSTER
execution or change strict full-C or game-identity counts.

## Prior work and provenance

Required deja queries `Poolrad RES_0464:02A5 video route 2 +07B3 +08CE
source object bounds` and `0464:02A5 route 2` found no matching prior
session. The merged postprocessor map supplied the entry, common return,
and caller frame. The reviewed [runtime Move contract](U6F_ENTRY6_0EC4.md)
identifies `0x0AF8:1685` as a five-word, overlap-safe copy with `RETF 0x0A`;
the route's specific push order and effects were decoded independently.
All address claims below come from ignored `START.EXE`, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

| Slice | Loaded image, exclusive end | START file, exclusive end | Bytes | SHA-256 |
| --- | --- | --- | ---: | --- |
| `RES_0464:+07B3..+08CE` | `0x04DF3..0x04F0E` | `0x04E2C..0x04F47` | 283 | `47505813c4fbaaac9d09797ba7df42c72f2fb76a8669657e2d5201f38f66f771` |

The slice has 101 complete instructions and 14 branch sites, each
targeting an instruction in this slice or the common epilogue at
`+0x08CE`. It contains one runtime Move call at `+0x07EE` and three
direct ES write sites at `+0x084E,+0x0871,+0x08A0`. The common epilogue
ends in `RETF 0x0E` at `+0x08D1`. The route inherits the full procedure's
far object pointer `[BP+6]`, raw selector byte `[BP+0x0A]`, ignored raw
byte `[BP+0x0C]`, mutable source-offset WORD `[BP+0x0E]`, and far source
pointer `[BP+0x10]`. The wrapper supplies initial source offset `0x11`;
the postprocessor initializes local data offset `[BP-0x1C]` to zero.

## Ordered route behavior

| Unit offset | Decoded effect | Model |
| --- | --- | --- |
| `+0x07B3..+0x07C7` | Read object header byte `+8`; zero skips all work. Otherwise set layer counter to 1. | Zero count needs only header byte `+8`; neither source nor auxiliary buffer is accessed. |
| `+0x07CF..+0x07EE` | Form source `far_source + source_offset` and destination `object + 0x17 + data_offset`, with 16-bit offset arithmetic. Push object header WORD `+0x11` as the copy length; call `Move`. | A host `memmove` copies that many bytes after a reached bounds check. Every layer reaches a Move call, including length zero. |
| `+0x07F3..+0x081F` | If both auxiliary far-pointer WORDs at object `+0x13/+0x15` are zero, skip masking. Otherwise calculate inclusive end `low16(data_offset + stride - 1)`; loop only if `data_offset <= end` unsigned. | A nonzero token enables the auxiliary route. The end and comparison use 16-bit arithmetic. |
| `+0x0824..+0x0852` | Compare primary byte's high nibble with the raw selector byte. On equality write `0xF0` to auxiliary[index]. | Same comparison and replacement. A selector above `0x0F` cannot match. |
| `+0x0852..+0x0875` | Compare primary byte's low nibble with the raw selector. On equality add `0x0F` to auxiliary[index], modulo 256. | Same conditional update; if both nibbles match, this follows the high-nibble replacement. |
| `+0x0875..+0x08A4` | Read auxiliary[index] even if neither comparison matched, invert it, AND primary[index], then write the primary byte. | Same reached read and primary write. Existing auxiliary masks can change primary data on a no-match byte. |
| `+0x08AF..+0x08CB` | Add object stride to data offset and source-offset frame WORD separately, modulo 65536; increment the byte layer counter and continue until it equals header count. | Report records both offsets and completed effects. Updates happen after the mask loop, so a host stop preserves the prior offsets. |

The primary index is relative to object data `+0x17`; the auxiliary index
is relative to its separate far allocation. The object count is read once
at entry. The stride is read at the Move call and again for each of the
two frame-offset updates. A nonzero auxiliary far pointer is checked
**after** Move, so missing auxiliary host mapping can stop with the
primary copy already visible. The report records the reached layer,
index, copy count, write counts, offsets, and stop site. Earlier effects
remain visible after a later host bounds stop.

## Host bounds and unresolved context

The original uses 16-bit far offsets and does not check source or object
allocation sizes. The C API maps far allocations to separate host byte
arrays from relative offset zero. It rejects a reached copy or byte access
that leaves those arrays or crosses a 16-bit segment boundary. It also
rejects null required mappings. These are host stops, not DOS branches.
The model assumes the source, primary object, and auxiliary arrays do not
alias. DOS far-pointer aliasing, invalid memory behavior, runtime register
or fault effects, modes 0/1, and whether the saved route reaches this
mode remain outside this slice. `memmove` follows the reviewed runtime
copy contract for the modeled disjoint allocations.

Synthetic tests cover zero count, two copy-only layers, a source-offset
WORD wrap from `0xFFFF` to zero between layers, zero stride,
high-only/low-only/both/no-match auxiliary effects, raw selector above
`0x0F`, and partial effects when a later Move or auxiliary byte access
hits a host bound. No original bytes are embedded.

## Reproduction

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/overlay/resident_464_mode2_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE
cmake -S . -B build -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build build
ctest --test-dir build --output-on-failure
```

The read-only audit hash-locks both parent procedure and this route,
checks source provenance, complete slice extent, branches, Move ABI,
frame updates, and ES writes, and emits only offsets, counts, and hashes.
Original game media remain ignored under `resources/`.
