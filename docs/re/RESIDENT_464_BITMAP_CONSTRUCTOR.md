# RES_0464:248E bitmap constructor

This is a **bounded C model** of the bitmap constructor reached by the merged
[RES_0464:0111 wrapper](RESIDENT_464_BLOCK_WRAPPER.md). Heap allocation,
runtime zero-fill, the live video-state byte, and the four-byte DOS
auxiliary pointer are host callbacks. The 16-bit arithmetic, branch gates,
allocation sizes, and direct object-header writes follow the decoded body.
This is neither a full-C translation nor a native LOAD MONSTER execution
claim; no strict-count or identity credit is proposed.

## Provenance and prior work

The first deja query, `Poolrad RES_0464:248E bitmap constructor heap video
mode`, had no exact match. The shorter `0464:248E` query recalled a
Claude overlay call trace. It was a lead; all claims below were rechecked
against ignored `START.EXE`, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The prior [bitmap release](RESIDENT_464_BITMAP_RELEASE.md) established the
paired lifetime and DS byte `[0x4A1E]`, while the
[LOAD MONSTER map](U1C_LOAD_MONSTER_0450_MAP.md) and
[unit-0x6F map](U6F_ENTRY6_0EC4_MAP.md) identified the runtime
`0xAF8:023F`/`16A8` calls as GetMem/FillChar.

| Procedure | Loaded image, exclusive end | START file, exclusive end | Bytes | SHA-256 |
| --- | --- | --- | ---: | --- |
| `RES_0464:248E` | `0x06ACE..0x06BD5` | `0x06B07..0x06C0E` | 263 | `3476a8ae8b3a3c1430c591b9922d493db2e4c475d542f089257464a82ce9d32f` |

The loaded bytes have contiguous unpacker source provenance. Capstone 5
decodes **95 complete instructions** through `RETF 0x0C`, followed
immediately by the next prologue at `RES_0464:2595`. There are four
branches at `0x24A4/0x2520/0x2527/0x2560` and six direct runtime calls:
GetMem at `0x24C0/0x2531/0x2565`, FillChar at
`0x24E1/0x255B/0x258A`. Those call sites are alternatives across paths,
not six calls on one invocation.

## ABI, arithmetic, and writes

The far frame consumes destination slot `[BP+6]`, raw byte
`[BP+0x0A]`, raw byte `[BP+0x0C]`, WORD `[BP+0x0E]`, WORD
`[BP+0x10]` (`RETF 0x0C`). The wrapper's call at
`RES_0464:+021F` pushes its block WORDs +0/+2, block byte +8, its own
`[BP+0x0C]` byte, and the far destination slot in that order.
The visual meaning of those input fields remains raw.

All formulas below truncate to 16 bits at the decoded AX store or shift:

```text
span_word = low16(word_0 * word_2) << 1
if first_read(DS:[0x4A1E]) != 0: span_word = low16(span_word << 1)
data_size = low16(byte_8 * span_word)
primary_size = low16(data_size + 0x17)
if (arg_a & 1) != 0:
    auxiliary_size = data_size >> 2  if second_read(DS:[0x4A1E]) == 1
                   = data_size       otherwise
```

| Unit offset | Original effect | Model |
| --- | --- | --- |
| `0x2494..0x24AE` | Multiply WORDs, shift AX, test `DS:[0x4A1E]`, optionally shift AX again. | 16-bit arithmetic; first `read_video` hook is reached before allocation. |
| `0x24AE..0x24CF` | Multiply byte `[BP+0x0C]` by span WORD, add `0x17`, GetMem, write returned far pointer to caller slot. | `allocate(primary_size)`; successful object becomes `*destination` before fill. |
| `0x24CF..0x2519` | FillChar zero of the primary allocation, then write WORDs to object +0/+2, span WORD to +0x11, and byte to +8. | `fill_zero` hook followed by exact little-endian writes. |
| `0x2519..0x2522` | Mask `[BP+0x0A]` with 1; zero returns. | Only bit 0 enables the auxiliary path. |
| `0x2522..0x2562` | Read video byte **again**. Value exactly 1 selects `data_size >> 2`; otherwise use full `data_size`. | Second reached `read_video`, then `allocate(auxiliary_size)`. |
| `0x2536..0x255B` or `0x256A..0x258F` | Store returned auxiliary far pointer at primary +0x13/+0x15, then FillChar zero of auxiliary allocation. | `attach_aux` maps the host pointer to the DOS field; then `fill_zero`. |

The first and second video reads can observe different values. The model
therefore calls the video hook separately at each reached instruction site.
It does not encode a host pointer into four DOS bytes or infer graphics
plane meaning from the raw count byte. The paired
`RES_0464:2595` release routine uses the same header +8/+0x11/+0x13
fields but remains a separate translation.

## Bounds, ownership, and integration

GetMem's internal allocator and runtime error behavior are unresolved.
The model reports host callback errors at the exact reached boundary and
retains prior object writes/allocations. `report.primary` and
`report.auxiliary` remain the host's responsibility after either a
successful call or a later stop. A wrapped `primary_size < 0x17`, an
undersized primary allocation, or an undersized auxiliary allocation is
rejected before an out-of-bounds zero-fill/write. Native DOS does not
perform these host guards.

The function consumes the merged wrapper's `make_object` signature
through a small caller adapter; the synthetic test exercises that
composition without modifying the wrapper API. A full adapter still
needs a DOS far-memory/object table, allocator semantics, the
`RES_0464:02A5` block processor, and live video state.

Tests cover no auxiliary allocation, quarter-size and full-size
auxiliary branches, separate video reads, 16-bit multiplication and
allocation-size wrap, hook-stop ownership, exact header offsets, and a
wrapper-to-constructor call on a synthetic 17-byte block.

## Reproduction

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/overlay/resident_464_constructor_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE
cmake -S . -B build -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build build
ctest --test-dir build --output-on-failure
```

The read-only audit hash-locks the constructor span and source provenance,
its full instruction extent, branch/call graph, direct write sites, and
wrapper caller. It emits only addresses, counts, and hashes. No original
bytes or media enter Git.
