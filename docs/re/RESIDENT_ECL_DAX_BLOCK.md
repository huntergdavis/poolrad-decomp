# Bounded resident ECL DAX block reader model

`pr_rec_res_0802_0716` models the DOS v1.3 resident entry
`RES_0802_0716` for [issue #74](https://github.com/huntergdavis/poolrad-decomp/issues/74).
It is a **path-complete bounded model with explicit file and runtime
boundaries**, not a full C translation. It adds no strict full-translation or
verified game-identity count: the file-open helper, Pascal runtime I/O and
allocation, fatal-error behavior, and the routine's game/library origin are
not established. The public load/release signatures match the callbacks in
the merged [overlay block-load model](ECL_BLOCK_LOAD.md); they do not claim to
implement that model's script-buffer effects.

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Loader provenance, extent, and ABI

The hash-locked `resident_loader_map.unpack` reconstructs loaded image
`0x08736..0x08969` from **564 contiguous copied bytes** at packed file
`0x0876D..0x089A0`; no byte in this span has fill provenance. Capstone 5.0.7
decodes 242 instruction starts and 11 direct branches, all to starts inside
the span. The 30 call sites comprise 22 far calls to `0xAF8` runtime entries,
seven local near calls, and one far call to `0x899:0x0000` on the oversized
packed-block path. There is one return at image `0x08967`, `RETF 0x12`.
The eight GAME.OVR fixup-backed far callers and two resident loader call sites
are recorded in the [callable ledger](resident_callable_ledger.csv). The
[overlay caller](ECL_BLOCK_LOAD.md) at unit `0x25:+0x03D3` independently
confirms the argument order and output use.

| DOS frame location | Observed input/output |
| --- | --- |
| `[BP+0x14]` | far Pascal name string |
| `[BP+0x10]` | far Pascal extension string |
| `[BP+0x0E]` | block ID in a word argument slot, consumed as a byte |
| `[BP+0x0A]` | far address of output unpacked-size word |
| `[BP+0x06]` | far address of output block far pointer |

The C function uses host C strings and a borrowed archive-view callback in
place of Pascal string and file records. The callback receives the first 20
characters of each string. The DOS helper's configuration path is **not**
assembled by the C function; it is part of the open callback's contract.

## Instruction paths and model boundary

| Loaded image address | Recovered effect | C model |
| --- | --- | --- |
| `0x0873D..0x0879A` | Copy both strings with a 20-character limit, build a path from `DS:0x547C` and the name, call file-open helper at `0x085ED`. | Truncate strings and call `open_archive`; missing file returns zero size and null pointer. |
| `0x0879C..0x087B0` | Failed open writes zero size and nil pointer; branch to the common return. | `MISSING_FILE`; no close after failed open. |
| `0x087B3..0x0882B` | Read each 9-byte directory row in order: byte ID, dword offset, word unpacked size, word packed size. | Parse the same row fields. |
| `0x0882B..0x08870` | Check packed size against `0x8CA0` **before** comparing that row's ID; oversize prints a message and calls `0x899:0`. | `PACKED_LIMIT_ERROR` for rows scanned before the first match; no fatal runtime call. |
| `0x08872..0x088D0` | Continue on ID mismatch while the directory position is below the header length; if exhausted, zero both outputs and close. | First matching ID wins; `MISSING_ID` zeros outputs and closes. |
| `0x088D3..0x0893D` | Allocate packed temporary rounded to eight bytes via local `0x08040`, allocate unpacked output via runtime GetMem, seek to header plus row offset, read packed bytes, close both file records via local `0x08707`. | Allocate both buffers, copy the selected packed bytes from a borrowed view, call `close_archive` before decode. |
| `0x08955..0x08964` | Decompress via local `0x0852F`, free packed temporary via local `0x08020`, retain output, return. | Decode, free packed buffer, transfer output ownership; release callback frees it with the returned size. |

The local decompressor at image `0x0852F..0x085E8` consumes controls
`0x00..0x7E` as `control + 1` literal bytes and `0x81..0xFF` as a repeated
following byte with count `256 - control`. Its input index does not advance
for `0x7F` or `0x80`; those controls would stall the DOS loop. The model
returns `RLE_STALL` in either case. It also returns host errors for malformed
directory bounds, truncated packed input, output overflow or underfill, zero
sizes, and allocation failure. These are **host safeguards**, not recovered
DOS error paths. The original decoder lacks those bounds checks; malformed
data can hang or overrun it. The model accepts at most 1 MiB of borrowed input.

The file-open helper `0x085ED` consults `DS:0x2614 + 0x1E * file number`,
`DS:0x547D`, and `DS:0x557C`; it opens runtime file records and reads the
header length. Those table and flag meanings, its failure behavior, and the
runtime GetMem/BlockRead/Seek/Close/FreeMem failure semantics remain unresolved.
The C callbacks stand at those boundaries. `0x899:0` is the observed fatal
call target, but its message and process effects are not translated. The
reader's archive role is verified by callers and data flow; whether this
object is game-authored or linked library code remains unresolved.

## Verification

Synthetic C tests cover literal and repeat decoding, name truncation,
first-match selection, the packed cap before ID comparison, missing file and
ID, close/allocate/free order and sizes, stalled controls, truncated literals
and repeats, overfill and underfill, invalid directory/range/size, allocation
failures, and invalid arguments. The standalone local audit compares each
block produced by this model with the existing portable DAX reader, using
ignored local files only; it prints aggregate counts and no bytes.

The eight `ECL1.DAX` through `ECL8.DAX` inputs contain **29 blocks**. Their
ordered-file digest is
`979544f439388e7a353f6ae1729fbd5a5973ceb4308a8920d69de7bd005f871e`
(SHA-256 of the concatenation of the eight individual SHA-256 digests, in
numeric file order). Packed sizes range 2,253..7,908; unpacked sizes range
2,171..7,679. No row exceeds the DOS packed cap, and no block contains a
stall control or raises a bounded decode error. The read-only C audit reports
**8 archives, 29 blocks, 29 exact byte matches, 176,265 decoded bytes**.
These are private comparisons reported only as counts and hashes.

```sh
cmake -S . -B /tmp/poolrad-issue-74-build \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-issue-74-build
ctest --test-dir /tmp/poolrad-issue-74-build --output-on-failure
/tmp/poolrad-issue-74-build/poolrad_resident_ecl_dax_block_audit \
  resources/DOS/POOLRAD/ECL{1,2,3,4,5,6,7,8}.DAX
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/resident_loader_map.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR --near \
  --json /tmp/poolrad-issue-74-loader.json
```

Before implementation, `deja "poolrad RES_0802_0716 ECL DAX block reader ABI
decompression"` returned only the current assignment rather than a prior
reader translation. The broader recall surfaced Claude's overlay loader
investigation. Reused research: the hash-locked [resident loader
map](RESIDENT_LOADER_MAP.md) and [callable ledger](resident_callable_ledger.csv)
for mapping/call sites, the merged [overlay caller note](ECL_BLOCK_LOAD.md)
for boundary ABI, and the existing [portable DAX reader](../../src/formats/dax.c)
as an independent private corpus comparison. The loader provenance, complete
reader branch/call census, callback ordering, and 29-block parity were checked
here. No executable, archive, decoded text, or pixels are committed.
