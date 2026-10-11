# Bounded resident DAX file-open helper model

`pr_rec_res_0802_05cd` is a path-complete **model with explicit
boundaries** of DOS v1.3 `RES_0802:05CD`. It is called by the resident
ECL DAX block reader `RES_0802:0716`. This work belongs to
[issue #83](https://github.com/huntergdavis/poolrad-decomp/issues/83);
the separate RLE decompressor in issue #82 and the reader files are outside
its scope. The helper's game-authored versus linked-library origin remains
unresolved, and this model does not increment a full-translation count.

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Loader, extent, and calls

The hash-locked `resident_loader_map.unpack` maps loaded image
`0x085ED..0x08706` to 282 contiguous copied START.EXE file bytes
`0x08624..0x0873D`, with no fill bytes. The body starts with a Pascal
prologue, ends in `RETF 0x14` at `0x08704`, and the next prologue
begins at `0x08707`. An independent Capstone 5.0.7 path walk reaches
118 instruction starts covering all 282 bytes. There is **one conditional
edge** (`0x0867E → 0x086FA`) and one unconditional edge
(`0x086F8 → 0x086FE`), both internal. This corrects the preliminary
issue description's count of two conditional edges.

The body has one local near call, `0x08679 → 0x0827D`, and 17
relocation-backed far calls into `0x0AF8` runtime code. The runtime
targets and site counts are `0x0B25` (2), `0x0B0B` (3),
`0x0B98` (4), `0x0C3B` (1), `0x13E2` (1), `0x1410` (1),
`0x04A9` (3), `0x14FB` (1), and `0x1563` (1). There are no
other reachable calls or returns.

The parent reader calls this entry directly at image `0x08795` and
tests AL at `0x08798`. Its `0x0AF8:0x0B0B` and
`0x0AF8:0x0B98` calls leave the destination far pointer on the
stack; that retained pointer supplies the first of this helper's five
arguments. The remaining four are pushed explicitly at
`0x0877D..0x08793`.

| Helper frame | Parent reader value | Helper use |
| --- | --- | --- |
| `[BP+0x16]` | Local `[BP-0x23D]`: `DS:0x547C` plus a 20-byte-capped caller name | First string, copied with a 40-byte cap |
| `[BP+0x12]` | Local `[BP-0x2A]`: 20-byte-capped caller extension | Second string, copied with a 40-byte cap |
| `[BP+0x0E]` | Address of local word `[BP-0x135]` | Header length word, written by BlockRead then increased by two |
| `[BP+0x0A]` | Address of first file record `[BP-0xB3]` | Passed to local open helper and later Seek |
| `[BP+0x06]` | Address of second file record `[BP-0x133]` | Assign, Reset, and BlockRead |

The frame offsets are relative to the file-open helper; the local
offsets in the middle column are relative to its reader caller.
`RETF 0x14` consumes those five far arguments.

## Complete local paths

| Image addresses | Observed effect | Model action |
| --- | --- | --- |
| `0x085F4..0x08634` | Copy each input Pascal string with maximum length `0x28`; assign and append the copies, retaining the destination pointer on the stack. | Build the first base-path-plus-extension value. |
| `0x0863A..0x0866F` | Read byte `DS:0x5376`; select the Pascal string at `DS:0x2614 + 0x1E × selector`; append a one-byte Pascal string made from `DS:0x547D`, then the Pascal literal at `CS:0x05CB`. | Read table and the literal's single data byte through callbacks; assemble the configured path. No literal contents are published. |
| `0x0866F..0x0867E` | Read byte `DS:0x557C`; pass both strings, that byte, and the first file record to local `0x0827D`; branch on AL. | `open_configured` callback. Zero returns AL=0 immediately. |
| `0x08680..0x086B7` | On nonzero AL, reread and recopy both input strings, build the second base-path-plus-extension value, Assign the second record, Reset it with record size one, call IOResult. | Rebuild, assign, reset, consume IOResult in order. |
| `0x086B7..0x086D4` | BlockRead two bytes from the second record directly into the caller's header word, with a local actual-count word; call IOResult. | `read_second` and `io_result` callbacks. The returned count and IOResult value are ignored by this body. |
| `0x086D4..0x08707` | Add two to that caller word with 16-bit wrap, zero-extend it to a Seek position for the first record, call IOResult, return AL=1. | Update the word, seek, consume IOResult, return one. |

The literal's length byte at loaded `0x085EB` equals one; its data byte
at `0x085EC` ends exactly before the helper prologue. Both bytes have
contiguous copy provenance at file `0x08622..0x08623`. The model reads
only the data byte through a callback, so its private value stays outside
the repository.

The four string-runtime entries above are checked against loaded
instructions as well as call shapes: `0x0AF8:0x0B25` caps a Pascal
copy at the supplied length, `0x0B0B` copies a Pascal string while
retaining its destination pointer, `0x0B98` appends up to the
255-byte length limit while retaining the destination, and
`0x0C3B` makes a one-byte Pascal string. The C model reproduces those
effects on synthetic strings. The local `0x0827D` call has its own
loop, file operation, and `RETF 0x0E`; this branch does not translate
it. Assign, Reset, IOResult, BlockRead, and Seek are labeled by argument
shape and the reviewed reader path, with their side effects delegated
to callbacks. Their exceptional behavior, the 30-byte table entries'
meaning, the two global bytes' meanings, and the code literal's
contents are not established.

On a completed zero-AL local-open result, the helper leaves the header
word and second file record untouched and returns AL=0. On the nonzero
result, it performs the ordered second-record and seek operations and
returns AL=1 even if an IOResult callback supplies a nonzero value.
Host callback failure stops the C model with AL unchanged and preserves
earlier effects; that is a host contract, not an extra DOS branch. The
model receives synthetic Pascal strings and named file-record callbacks,
so it does not claim far-memory alias or complete runtime parity.

## Verification and prior work

Synthetic tests cover both AL paths, five-argument path construction,
second reads after a callback changes the source strings, 40-byte
copy caps, 255-byte append saturation, maximum table selector, header
wrap and seek value, ignored nonzero IOResult values, all 13 boundary
failure positions, and invalid host arguments. The focused test passes
under `-Wall -Wextra -Wpedantic -Werror`; full CTest and public hygiene
results are recorded in the PR.

Before reimplementation, `deja "RES_0802_05CD DAX file open issue 83
bounded model caller ABI"` found no exact prior translation. The
broader recall found the issue #74 assignment and earlier overlay
investigation. Reused leads: the merged
[loader map](RESIDENT_LOADER_MAP.md),
[overlay block-load note](ECL_BLOCK_LOAD.md), the
[reviewed reader model](RESIDENT_ECL_DAX_BLOCK.md), and the
[issue #74 coordination proposal](https://github.com/huntergdavis/poolrad-decomp/issues/74#issuecomment-6110293192).
The hashes, source span, full path graph, call fixups, caller stack,
runtime string operations, and side-effect order were independently
rechecked against the ignored DOS files. No game text, executable,
archive, configuration value, or decoded media enters Git.
