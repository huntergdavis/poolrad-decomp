# RES_0464:0111 block-to-bitmap wrapper

This is a **bounded host model**, not a full C translation or proof of live
LOAD MONSTER execution. It follows the wrapper's three decoded branch targets
and its direct field copies, while keeping the bitmap constructor, bitmap
release, local block processor, heap free, and runtime key flush as explicit
callbacks. No game-specific identity or strict full-C count is claimed.

## Reused research and input

Before implementation, deja query `Poolrad RES_0464:0111 wrapper 0802:0716
output ownership zero size branch` had no exact match. The shorter
`0464:0111` query recalled a Claude overlay call trace and a p6 prologue
decode; both were leads only. The merged
[resident block route](LOAD_MONSTER_RESIDENT_BLOCK_ROUTE.md),
[bounded DAX reader](RESIDENT_ECL_DAX_BLOCK.md),
[bitmap release](RESIDENT_464_BITMAP_RELEASE.md), and
[unit 0x3CE entry-4 caller](U3CE_ENTRY4_01C8.md) established the API and
static call route. The [LOAD MONSTER map](U1C_LOAD_MONSTER_0450_MAP.md)
independently identifies runtime `0xAF8:0x0A3F` as `Move`. All wrapper
addresses below were decoded afresh from ignored `START.EXE`, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

| Procedure | Loaded image, exclusive end | START file, exclusive end | Bytes | SHA-256 |
| --- | --- | --- | ---: | --- |
| `RES_0464:0111` | `0x04751..0x048E5` | `0x0478A..0x0491E` | 404 | `ac5dab193845bac0fc67613a82dc60b7148a930e9d1d9f319129e32a7f24271a` |

The hash-locked span decodes to **154 complete instructions**, ending exactly
at the next prologue `RES_0464:02A5`. It contains three branches at
`+0x0136`, `+0x0171`, `+0x0173` and fifteen direct calls. The prior
route audit also checks contiguous unpacker source provenance. This note and
the C files contain no DOS bytes or private names.

## Frame and reached effects

`RETF 0x0E` consumes a far destination pointer slot `[BP+6]`, raw byte
`[BP+0x0A]`, raw byte `[BP+0x0C]`, the **low byte** of ID WORD
`[BP+0x0E]`, and far Pascal name `[BP+0x10]`. The C model receives these
as a host object slot, two bytes, one WORD, and a length-prefixed buffer.
The private extension at `CS:+010C` is supplied by the host and is not
published.

| Unit offset | Decoded action | C boundary |
| --- | --- | --- |
| `+0x0118..+0x012C` | `StrStore` copies name with cap 255. | C adapter sends its first at most 20 bytes, the merged reader's later name cap. Embedded NUL is rejected at the reader boundary. |
| `+0x012C..+0x0141` | OR the two far pointer words; if nonzero, call `RES_0464:2595` on the destination slot. | `release_prior` only for a non-null host object. It may already have cleared the destination when any later step stops. |
| `+0x0141..+0x016C` | Load private extension and call `RES_0802:0716` with name, ID low byte, local size and pointer outputs. | `read_block` has the merged `pr_rec_res_0802_0716` signature. Its C status is a host stop; DOS callers inspect size instead. |
| `+0x016C..+0x0176` | If local size is zero, jump to `+0x029F` return. | No constructor, helper, block free, or flush call. Earlier release remains effective. |
| `+0x0176..+0x0208` | Six `Move` calls copy lengths 2, 2, 2, 2, 1, 8 from block offsets 0, 2, 4, 6, 8, 9. | The model requires at least 17 bytes before reading. DOS does not check this bound. |
| `+0x0208..+0x0222` | Call `RES_0464:248E` with first two WORDs, byte +8, caller byte `[BP+0x0C]`, and destination slot. | `make_object` creates a writable object with at least 17 header bytes. Its heap allocation and separate video-state behavior remain unresolved. |
| `+0x0222..+0x0261` | Write WORDs from block +4/+6 to object +4/+6, then `Move` eight bytes from block +9 to object +9. | The C model performs these byte-exact writes after successful construction. |
| `+0x0261..+0x0289` | Call `RES_0464:02A5` with source block, constant offset `0x11`, caller bytes `[BP+0x0C]` and `[BP+0x0A]`, and current object pointer. | `process_block` retains the remaining block interpretation and effects. |
| `+0x0289..+0x029F` | `FreeMem(block, size)`, then `RES_0899:0104`, then return. | `release_block` matches the merged reader release callback; `flush_899_0104` is explicit. |

The six copied fields are a **17-byte prefix**; the object constructor's
other header fields and the data after source offset `0x11` are not
interpreted here. `RES_0464:248E` allocates a bitmap header and data
according to runtime video state, and `RES_0464:02A5` continues beyond the
next prologue. Their behavior is deliberately outside this model.

The merged unit-0x3CE entry-4 path statically calls this wrapper at `+045B`
and `+04C4` with the original ID byte and ID + `0x80` respectively.
That path is modeled with host callbacks; a native saved-route call remains
unproved. p6 confirmed its unit-0x6F entry-6 adapter calls the resident
reader directly and has no ownership or file overlap with this wrapper.

## Host stops and ownership

Callback presence is checked **when reached**. A missing hook or nonzero
callback result stops after all prior calls and writes. The report records
the exact reached-call sequence, stop site, reader status, and any block
still held before the DOS `FreeMem` site. In that case the host remains
responsible for releasing the block. A successful read with size zero exits
without freeing the block; the merged reader returns a null pointer for its
non-OK/zero-size paths. A nonzero read shorter than 17 bytes or a constructor
returning less than 17 writable bytes is rejected as a host bound. These
guards, callback errors, and C-string conversion are model limits, not
claims about native exception handling.

The synthetic tests cover an old-object release followed by zero size,
the full 17-byte copy and low-byte ID, missing and failing reached hooks,
short block and embedded-NUL guards, and a fixture that invokes the merged
reader on a synthetic one-entry DAX archive. No original block data is used.

## Reproduction

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/overlay/load_monster_resident_block_route.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR
cmake -S . -B build -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build build
ctest --test-dir build --output-on-failure
```

The route script hash-locks the wrapper span, source mapping, direct reader
call, and the overlay callers. The private Capstone check additionally
confirmed the 154-instruction extent, three branch sites, and fifteen calls.
