# RES_0464:02A5 video-state-1 route

This is a **bounded C model of one route** inside the merged
[postprocessor map](RESIDENT_464_POSTPROCESS_MAP.md). Entry assumes its
earlier nonzero-object and raw video-byte-equals-1 gates have passed.
The parent 604-instruction procedure, native LOAD MONSTER reachability,
and game-specific ownership remain unresolved. No strict full-C or
game-identity count change is claimed.

## Prior work and provenance

Required deja query `Poolrad RES_0464:02A5 video route 1 +05F6 +07A9
Move helper writes` found no match. A shorter `0464:02A5 route 1` query
recalled this lane's prior route-2 handoff; it supplied context only, not
route-1 instruction evidence. The merged [route-2 note](RESIDENT_464_MODE2.md),
[bitmask helper](RESIDENT_464_BITMASK.md), and reviewed
[runtime Move contract](U6F_ENTRY6_0EC4.md) supplied API and callee leads.
The route was decoded independently from ignored `START.EXE`, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

| Slice | Loaded image, exclusive end | START file, exclusive end | Bytes | SHA-256 |
| --- | --- | --- | ---: | --- |
| `RES_0464:+05F6..+07A9` | `0x04C36..0x04DE9` | `0x04C6F..0x04E22` | 435 | `d91ed4da7d2d1ff7d23a8f977f26bdde07b3d93067c91bcb9a746b31646cade1` |

The hash-locked slice has contiguous unpacker source provenance and 162
complete instructions. Its 27 branch sites target instructions in the
slice or the route exit at `+0x07A9`; that exit jumps to the parent's
common `RETF 0x0E` epilogue. Direct calls are runtime
`0x0AF8:1685` Move at `+0x065E` and near `RES_0464:0000` bitmask at
`+0x067B`. Three direct ES writes occur at `+0x068C,+0x070D,+0x0776`.
The raw table read is DS `+0x25F6` indexed by `0..3`; its original
four bytes are not published.

## Frame and ordered effects

The route inherits far object pointer `[BP+6]`, raw selector byte
`[BP+0x0A]`, ignored byte `[BP+0x0C]`, mutable source-offset WORD
`[BP+0x0E]`, and far source pointer `[BP+0x10]`. The wrapper supplies
source offset `0x11`; the parent initializes data offset `[BP-0x1C]`
and auxiliary index `[BP-0x1E]` to zero before video dispatch.

| Unit offset | Decoded behavior | Bounded C representation |
| --- | --- | --- |
| `+0x05F6..+0x060A` | Read header count byte `+8`. Zero exits; otherwise layer counter starts at 1. | Zero count requires only object bytes through `+8`. |
| `+0x0612..+0x064B` | Nested WORD loops: x runs `0..low16(width-1)` from header `+0`, then y runs `0..low16(height-1)` from `+2`, inside each layer. | Width or height zero underflows to an inclusive 65,536-iteration range. A host pixel budget bounds work. |
| `+0x064B..+0x0663` | Copy four source bytes from `far_source + source_offset` to a local stack buffer through Move. | Copy four bytes after a reached source bound; the runtime Move is a reviewed copy. |
| `+0x0663..+0x0692` | If auxiliary far-pointer WORDs at object `+0x13/+0x15` are nonzero, call merged `RES_0464:0000` on the four copied bytes and raw selector, write its AL mask at auxiliary index, then increment that WORD index. | Invoke merged `pr_r464_bitmask`, write once, and retain prior Move if host auxiliary mapping is missing. |
| `+0x0692..+0x072A` | Walk eight copied nibbles in high/low order with bit mask `0x80` shifting right once each nibble. For each of four table entries, nonzero `(table[j] & nibble)` ORs the bit into primary data byte `data_offset+j`. | Host supplies four raw table bytes; a short/missing table stops when its entry is first reached. Primary writes are checked individually. |
| `+0x072A..+0x0780` | Recheck auxiliary pointer. If present, for each `j=0..3` read auxiliary byte at `low16(auxiliary_index-1)`, invert it, AND primary data byte `data_offset+j`, and write primary. | Read the last mask for each primary byte. Previous OR writes remain visible if an AND access stops. |
| `+0x0780..+0x07A9` | Add four to data offset and source-offset frame WORDs; advance y, then x, then layer at their inclusive bounds. | Offset additions wrap at 16 bits after a completed pixel. Report preserves current loop coordinates and effect counts on a host stop. |

Primary data is relative to object header `+0x17`; the auxiliary byte
index is relative to its separate far allocation. The helper call and
auxiliary write occur **before** table-driven OR writes. The final
auxiliary mask read occurs even when no table entry matched. A raw
selector above `0x0F` retains the merged bitmask helper's 8-bit shift
behavior; it is not clamped to a nibble.

## Host stops and limits

The original uses 16-bit far offsets without allocation-size checks. The
C API maps source, object, and auxiliary far allocations to disjoint host
arrays from relative offset zero. It rejects a reached read/write that
leaves an array or crosses a 16-bit segment boundary. The four private
table bytes are a host input; their content is neither guessed nor
embedded. `max_pixels` limits the potentially enormous zero-dimension
loops. Missing mappings, table entries, and budget exhaustion are host
stops, not DOS branches. Earlier copies, auxiliary writes, primary OR/AND
writes, and completed frame-offset updates remain visible after a later
stop. Far-pointer aliasing, invalid DOS memory/fault effects, other video
modes, and live saved-route execution are outside this bounded model.

Synthetic tests cover zero count, table-driven OR bits, merged bitmask
composition, auxiliary AND masking, nested x/y progression, zero-width
underflow stopped by the pixel budget, and partial effects after reached
source, table, and auxiliary mapping failures. Tests use only synthetic
bytes.

## Local reproduction

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/overlay/resident_464_mode1_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE
cmake -S . -B build -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build build
ctest --test-dir build --output-on-failure
```

The read-only audit hash-locks the parent and this route, checks
instruction extent, branch targets, Move and bitmask calls, the DS table
reference, ES writes, and frame updates. It emits only offsets, counts,
and hashes; original game media remain ignored under `resources/`.
