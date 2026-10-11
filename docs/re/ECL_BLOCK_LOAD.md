# ECL block load: unit 0x25 + 0x0341 and its resident boundary (issue #68)

**Scope.** The DOS v1.3 routine that loads one area script block into the engine's script
buffer, called on the new-game branch of the gameplay loop ([GAMEPLAY_PATH.md](GAMEPLAY_PATH.md)
step 7). Inputs: `GAME.OVR` sha256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`
and `START.EXE` sha256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
digest-checked before decoding; the resident image expanded in memory with the merged
loader module. Only offsets, lengths, immediates, counts and hashes appear here. An exact
`deja` query for prior block-loader work returned no session. Translation:
`pr_rec_ecl_load_block` (FUNCTION_LEDGER.md section 1.15, function 18, a **path-complete
model with explicit boundaries**, not counted as a full translation).

## 1. Bounds and census, independently re-derived

| item | value |
| --- | --- |
| identity | GAME.OVR `0x07814`, unit `0x25 + 0x0341`, public entry 4, run-time stub `0x2B:0x34` |
| extent | `+0x0341..+0x0410`, 208 bytes, 89 instructions, ends in `retf 2` (one byte argument, `[bp+6]`) |
| branches | one: `+0x03DC jb +0x035F` (backward, on `size < 2`) |
| call sites | 12: runtime `0xAF8` 8 (`0x16A8`, `0x0B0B` three times, `0x1366`, `0x0B98`, `0x1685`, `0x0254`), resident 2 (`0x709:0x0626`, `0x802:0x0716`), overlay 2 (unit `0x3b0` entry 3, `+0x06FB`, twice) |
| callers (fixup-confirmed) | unit `0x1c + 0x3358` (the gameplay loop, map id) and unit `0x1c + 0x0C50` (inside the handler at `+0x0BD1` that also rewrites the map id) |
| locals | `0x112` bytes: `[bp-6]` digits string, `[bp-8]` size, `[bp-0xC]` far pointer, `[bp-0x22]`, `[bp-0x10C]`, `[bp-0x111]` string temporaries |
| constants in code | `+0x0322` 21 characters (wait message), `+0x0338` 3 characters, `+0x033C` 4 characters (the name parts of the `ECL<n>.DAX` pattern; the extension constant is lower case) |

## 2. Instruction paths

| offsets | effect | boundary in the C model |
| --- | --- | --- |
| `+0x0348..+0x0355` | runtime `0xAF8:0x16A8` with (`[0x49DE]^`, `0x1E00`, 0): fill the script buffer | direct zeroing of `PR_REC_ECL_SCRIPT_BUFFER_SIZE` bytes |
| `+0x035A` | `[0x557C] = 0` | `state->b557c = 0` |
| `+0x035F` | unit `0x3b0` entry 3: `0x709:0x05EB` with (0, `0x18`, 0, `0x28`), a row clear | `clear_row()` (optional) |
| `+0x0364..+0x037C` | `0x0B0B` loads the 21-character constant into `[bp-0x22]` and leaves it as the argument of `0x709:0x0626` with (0, `0x18`, `0x0F`): the wait message | `show_wait()` (optional, no text) |
| `+0x0381..+0x0396` | `0x1366` with (`0:[0x5376]`, width 0, `[bp-6]`, 1): Str of the file number | decimal digits |
| `+0x039B..+0x03B0` | `0x0B0B` loads the 3-character constant into `[bp-0x10C]`; `0x0B98` appends `[bp-6]` | `"ECL" + digits` |
| `+0x03B5..+0x03C0` | `0x0B0B` loads the 4-character constant into `[bp-0x111]` | `PR_REC_ECL_BLOCK_EXT` |
| `+0x03C5..+0x03D3` | push block id, `&[bp-8]`, `&[bp-0xC]`; `0x802:0x0716` (five arguments with the two strings, `retf 0x12` on the callee) | `load_block(name, ext, id, &size, &block)` |
| `+0x03D8..+0x03DC` | `size < 2` loops to `+0x035F` without releasing | retry loop; `RETRY_LIMIT` after `max_attempts` |
| `+0x03DE..+0x03F3` | `0x1685` with (`block + 2`, `[0x49DE]^`, `size - 2`): Move | copy; `OVERFLOW` if `size - 2 > 0x1E00` (the engine has no check) |
| `+0x03F8..+0x0401` | `0x0254` with (`block`, `size`): FreeMem | `release(block, size)` |
| `+0x0406` | unit `0x3b0` entry 3 again | `clear_row()` |
| `+0x040B..+0x040E` | epilogue, `retf 2` | `OK`; the state's script pointers are bound to the buffer |

Nothing else is read or written: the PC, the entry addresses and the windows are untouched
(the initializer, function 17, runs next in the gameplay loop). The runtime routines are
identified by argument shape only (fill, string load, Str, concat, Move, FreeMem).

## 3. The resident boundary, decoded read-only (resident lane, not translated here)

`RES_0802_0716` (Codex's [resident ledger](resident_callable_ledger.csv), image `0x08736`,
`retf 0x12`, 564 bytes to its epilogue at `0x08964`) takes (name base `[bp+0x14]`, extension
`[bp+0x10]`, block id `[bp+0xE]`, out size `[bp+0xA]`, out pointer `[bp+6]`):

- `0x0873D..0x0879A`: copies both strings (limit 20), builds the path from the runtime
  string at `DS:0x547C` (beyond the file image, so filled at run time; the configuration path
  the DOS oracle fixed on 2026-10-11) and the name, and opens the file through the helper at
  image `0x085ED` (which also consults a 30-byte record table at `DS:0x2614 + 0x1E *
  [0x5376]`, the byte `[0x547D]`, the byte `[0x557C]`, and reads the header length word).
  On failure: size 0, pointer nil, return.
- `0x087B3..0x0882B`: per header entry, BlockRead of 1 + 4 + 2 + 2 bytes: id, offset, the
  caller's size (unpacked), `[bp-0x2C]` (packed). `0x0882B`: a packed size above `0x8CA0`
  prints a message with the number (`0x1366`, constant at `cs:0x070C`, helper `0x081B3`) and
  exits through `0x899:0x0000`.
- `0x08872..0x088A1`: id mismatch continues while the file position (`0x1593`) is below the
  header length; otherwise size 0, pointer nil, close (`0x08707`), return.
- `0x088D3..0x08961`: GetMem of `(packed + 7) & ~7` into `[0x557E]`, GetMem of the unpacked
  size into the out pointer, Seek to header length + entry offset, BlockRead of the packed
  bytes, close, decompress (`0x0852F`), FreeMem of `[0x557E]`.
- `0x0852F..0x085E8` (decompressor): control byte `0x00..0x7E` copies that many plus one
  literal bytes; `0x81..0xFF` (negative) fills its negation with the following byte; the loop
  runs while the input index is below the packed size. Control bytes `0x7F` and `0x80` take
  neither branch and do not advance the input index, so the original would not terminate on
  them. This is the scheme of `src/formats/dax.c` (`pr_dax_rle`, Gold Box Explorer), which
  the private audit below uses as the boundary implementation.

Contract used by the model: success means `size >= 2` and a block of `size` bytes whose first
two bytes are the record prefix; failure means size 0 and a null pointer. The resident
routine's own paths (missing file, missing id, oversize packed block, decompression) stay
with the resident lane.

## 4. Measured limits (counts only)

The eight `ECL<n>.DAX` files hold 29 blocks (ids as in ECL_CONTRACTS.md section 1; file 3
holds ids 0, 8, 11, 14). Unpacked sizes range 2,171..7,679 bytes, packed sizes at most 7,908.
For every block `unpacked - 2 <= 0x1E00`, so the unchecked Move never exceeds the buffer on
the shipped data, and no packed size reaches the `0x8CA0` cap. Block 0 of file 3 (the
new-game block, GAMEPLAY_PATH.md section 3) unpacks to 7,473 bytes.

## 5. Private hash-checked integration audit

A scratch program (not committed) opened each `ECL<n>.DAX` with `pr_dax_open`, unpacked
each block with `pr_dax_read`, served it through `load_block`, ran `pr_rec_ecl_load_block`
and then `pr_rec_ecl_script_init`, and compared the five entry addresses with
`pr_ecl_open`'s `entry_address[]` on the same unpacked block. Result: 29 blocks, 29 loaded with one attempt and one release each, 145 of 145 entry addresses equal, 29 of 29 names equal `ECL<n>`; the new-game block (file 3, id 0) unpacks to 7473 bytes, 7471 bytes land in the script buffer, the PC after the initializer is `0x9914`, and the copied bytes hash to sha256 `632920ceca62c04c8188e76baccbe549f5eed86874b0c802309f0005f61df8d9` (reproducible from the owner's files with the recipe in section 2 of ECL_CONTRACTS.md plus this routine).

## 6. Tests (synthetic)

Name selection for file numbers 0, 3, 10, 255 and the extension constant; the call sequence
(clear, wait, load, release, clear); copy of `size - 2` bytes from offset 2 with the rest of
the buffer zero and nothing written past `0x1E00`; release once with the same pointer and
size, after the copy and before the final clear; retry on size 0 and on size 1 (no release of
the short block), attempt count and sequence; a failing callback treated as size 0; the retry
bound with and without the attempt counter; the exact-fit and one-over bounds; absent optional callbacks; composition with the
initializer on a synthetic block head; null arguments.

## 7. What remains unresolved

The resident loader and file-open helper are decoded, not translated; the meanings of
`[0x557C]` and of the `DS:0x2614` table are not established; the runtime routines are
inferred from argument shape. The model adds two statuses the engine lacks (`RETRY_LIMIT`,
`OVERFLOW`), which is why it is not a full translation.
