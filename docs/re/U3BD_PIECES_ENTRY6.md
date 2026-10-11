# Unit 0x3bd entry 6 (+0x0E08, stub 0x3D5:0x003E): path-complete local-body model (issue #99)

`src/recovered/overlay_u3bd_pieces.c` models DOS v1.3 `GAME.OVR` unit
`0x3bd` public entry 6, the routine called by the merged class-1 store hook
(unit 0x25 +0x0A74, cases 0x322/0x324/0x326, PR #96) and by the LOAD FILES /
LOAD PIECES handler (unit 0x1c +0x0C74, PR #97's `load_pieces(ctx, id,
slot)` boundary). Classification: **path-complete local-body model with
callback boundaries**, not a full translation (FUNCTION_LEDGER.md §1
vocabulary): the routine's own instruction paths are reproduced; its
sixteen far-call sites to thirteen distinct targets (eight runtime
routines at nine sites, four resident routines at five sites, one overlay
routine at two sites) are six callbacks (string construction folded into
the loader callback's inputs). The strict full-C count is unchanged.
`ecl_helpers.c/h` and the merged hook API are untouched.

Two earlier drafts of this note were corrected after PM review: the
call-site count (fourteen → sixteen), and the notice path, which an
earlier draft treated as continuing into the copy loop. The PM's
independent decode of `START.EXE` shows `RES_0899_0000` (loaded
`0x08990..0x089C2`) calling runtime `0xAF8:0x00E9` at `0x089BC`, which
reaches `mov ax,[0x441C]; mov ah,0x4C; int 0x21` at loaded
`0x0B11E..0x0B123`: a DOS terminate. The notice path therefore never
returns, its `retf` is unreachable, and nothing after `+0x0EBC` executes on
that path. GAMEPLAY_PATH.md already names `0x899:0x0000` the exit routine.

`deja "unit 0x3bd entry 6 +0x0E08 0x3D5:0x003E LOAD PIECES"` matched only
this worker's hook session. Reused with citation and re-checked from the
bytes: the hook's call-site evidence (PR #96 note), PR #97's handler
decode (pending, cited as a lead for the unit 0x1c sites, which are
re-decoded below), and [ECL_BLOCK_LOAD.md](ECL_BLOCK_LOAD.md), whose
block loader `+0x0341` uses the same Str / string-load / concat / resident
`0x802:0x0716` / Move / FreeMem sequence and whose read-only decode of
`RES_0802_0716` (five arguments, `retf 0x12`, size 0 and nil pointer on
failure) fixes that callee's contract.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

Unit `0x3bd`: file offset 190,711, code size 4,619, eight public entries,
run-time segment `0x3D5`; entry 6 maps to `+0x0E08` (stub `0x3D5:0x3E`).

| Item | Value |
| --- | --- |
| extent | `+0x0E08..+0x109C`, **661 bytes, 258 instructions**, one `retf 4`; the ledger bound of 703 includes 42 trailing bytes that are a 4-character and a further length-prefixed printable constant, not code |
| span SHA-256 | `88f8bcf2ed9b5333edc375d9acf1aa1e3f8d4203b1f5f4da1cf48297af60f88b` |
| frame | `sub sp, 0x118`; `[bp+8]` = id (first pushed), `[bp+6]` = slot (second pushed); both read as bytes only (`mov al, [bp+n]` / `cmp byte ptr`) |
| branches | 22, all inside the span; backward: `+0x0F1A→+0x0EDF` (copy loop), `+0x100C→+0x0F9F` (column loop), `+0x1012→+0x0F95` (row loop), `+0x105E→+0x0F49` (piece loop) |
| call sites | 16 to 13 distinct targets: runtime `0xAF8` ×9 (`0x1366`, `0x0B0B` ×2, `0x0B98`, `0x0919`, `0x084A`, `0x1685`, `0x0254`, `0x0DC0`); resident ×5 (`0x802:0x0716`, `0x464:0x00FF` ×2, `0x899:0x0059`, `0x899:0x0000`); overlay ×2 (`0x428:0x25` = unit `0x40c` entry 1, `+0x003B`) |
| constants in code | `+0x0DB4` 7 characters and `+0x0DBC` 4 characters (name parts), `+0x0DC1` 38 characters (notice text), `+0x0DE8` a 32-byte set with exactly the members {1, 2, 3} |
| data-segment words | `[0x5376]` (file number), `[0x2722..0x2728]` (table indexed by slot) and `[0x272C]` (threshold), far pointer `[0x6A58]` (pieces buffer), words `[0x6AAE + 4k]`/`[0x6AB0 + 4k]` |

## Callers (eight fixup-confirmed far calls; stub `0x3D5:0x3E`)

| Site | Pushed (id, slot) | Context |
| --- | --- | --- |
| unit 0x25 `+0x0C26` / `+0x0C49` / `+0x0C6C` | (`value & 0x7F`, 1 / 2 / 3), AH = 0 in both words | the merged hook, window addresses 0x6E22/24/26 with value > 0x80 |
| unit 0x1c `+0x0D31` | (0, 1) | handler `+0x0C74`, when its byte `[bp-3]` is 0x7F |
| unit 0x1c `+0x0D58` then `+0x0D64` | (`[bp-3]`, 1) then (`[bp-1]`, 3) | when window-0 words at `[0x49D2]+0x1CE` and `+0x1D0` are both non-zero |
| unit 0x1c `+0x0D90` | (`[bp-4+i]`, i) in a loop until the byte is 0xFF | otherwise |
| unit 0x6f `+0x1C94` | (low byte of `[0x6AAE+4k]`, low byte of `[0x6AB0+4k]`) | replays a recorded slot when its first word is not negative (`jl` skip) |

The unit 0x6f site shows the purpose of the slot table this routine
writes: each slot records the (id, slot) it was loaded with so the pair can
be replayed. The unit 0x1c pushes use `mov al, …; push ax` with whatever
AH holds; the callee reads bytes only, so that is harmless.

## Instruction paths (local effects in order; `id` = `[bp+8]`, `slot` = `[bp+6]`)

| Offsets | Effect | Model |
| --- | --- | --- |
| `+0x0E0F..+0x0E1B` | `cmp byte [bp+6],1; jb exit; cmp byte [bp+6],3; ja exit` | slot outside 1..3: return with no effect |
| `+0x0E1E..+0x0E33` | `0xAF8:0x1366` with (`0:[0x5376]`, width 0, `[bp-0x12]`, 1): Str of the file number | `file_number` passed to `read_block` |
| `+0x0E38..+0x0E5D` | `0x0B0B` loads the 7-character literal, `0x0B98` appends the digits, `0x0B0B` loads the 4-character literal: the two string arguments | name parts documented by length; the host builds the name |
| `+0x0E62..+0x0E70` | push id byte, `&[bp-2]` (size), `&[bp-6]` (pointer); `0x802:0x0716` (nine words with the two strings) | `read_block(ctx, file_number, id, &block, &size, &capacity)` |
| `+0x0E75..+0x0E91` | if size == 0, or `size / 0x30C + slot > 4` (16-bit `div`), fall into the notice path; otherwise jump to `+0x0EC1` | the same test |
| `+0x0E93..+0x0EB7` | `0x464:0x00FF`; `0x0919` with (`[0x8608]`, the 38-character literal, 0); `0x084A`; `0x899:0x0059` (result to `[bp-0x13]`, never read); `0x464:0x00FF` | `error_notice(ctx)` |
| `+0x0EBC` | `0x899:0x0000`: **DOS terminate** (through runtime `0xAF8:0x00E9`, `int 0x21` with AH = 0x4C). Does not return; the fall-through into `+0x0EC1` is dead on this path | `exit_program(ctx)`, then `PR_U3BD_EXITED` immediately |
| `+0x0EC1..+0x0EDA` | `[bp-0xA] = [0x2722 + 2·slot] − [0x2724]`; offset = 0; i = 1 | `delta` = low byte of `table[slot] − table[1]` |
| `+0x0EDF..+0x0F1A` | **do** `0x1685` Move(block + offset, `[0x6A58]` + (slot + i − 1)·0x30C − 0x30C, 0x30C); offset += 0x30C; i++ **while** offset < size | copy piece j into slot `slot + j` at `(slot + j − 1)·780`; runs at least once |
| `+0x0F1C..+0x0F25` | `0x0254` FreeMem(block, size) | `release(ctx, block, size)` |
| `+0x0F2A..+0x0F41` | count = i − 1 (byte); if 1 > count skip to the tail | `count = ceil(size / 780)`, at least 1 here |
| `+0x0F44..+0x0F62` | for i = 1..count: `0x0DC0` set test of (slot + i − 1) against the set at `+0x0DE8`; not a member → next piece | `pr_u3bd_slot_in_set`: members {1, 2, 3}; slot 4 is skipped |
| `+0x0F65..+0x0F8F` | `[0x6AAE + 4·s] = 0xFFFF`, `[0x6AB0 + 4·s] = 0xFFFF` for s = slot + i − 1 | the same words |
| `+0x0F8F..+0x1012` | for row 1..5, for k 0..0x9B: byte at `[0x6A58]` + s·0x30C + row·0x9C + k − 0x3A8 (= piece s, offset (row−1)·156 + k); if (zero-extended byte) ≥ `[0x272C]` (word compare): byte += delta | identical walk over the 780 bytes in order |
| `+0x1014..+0x1051` | if count > 1: if id == 0 then id = 10 (persistent in `[bp+8]`); push id·10 + i; push s; `0x428:0x25`. Else push id; push s; `0x428:0x25` | `piece_loaded(ctx, arg, s)` with `arg` as computed; AH is 0 in both pushes |
| `+0x1056..+0x105E` | until i == count | |
| `+0x1061..+0x1093` | if id == 10 then id = 0; `[0x6AAE + 4·slot] = id`; `[0x6AB0 + 4·slot] = slot` | the same words; `final_id` reported |

Side-effect order on the normal path: read, copies, release, per piece
(mark, remap, announce), final slot words. On the notice path: read,
notice display and key read, program exit; no copy, release, remap or
table write ever follows, because `0x899:0x0000` does not return.

**Bounds that follow from the gate.** The normal path requires
`floor(size / 780) + slot ≤ 4` with size ≥ 1, and the copy count is
`ceil(size / 780) ≤ floor(size / 780) + 1`, so the last slot written is
`slot + count − 1 ≤ 4`: the four-slot buffer cannot be overrun. Slot 4 is
reached only by a trailing partial piece (size not a multiple of 780, for
example 781 bytes from slot 3), and for such sizes the original's whole
780-byte Move reads past `size` inside its heap block; the model asks the
host for the block's readable `capacity` and reports `BLOCK_BOUNDS`
instead of reading past it. These are the only undefined reads left; the
nil-pointer copy and slot overrun an earlier draft described cannot occur,
since both conditions take the terminal notice path.

## Dependencies named

| Call-out | Identity | Modelled as |
| --- | --- | --- |
| `0xAF8:0x1366`, `0x0B0B`, `0x0B98` | runtime Str / string load / concat by argument shape (same shapes as ECL_BLOCK_LOAD.md) | inputs of `read_block` (file number; literal lengths 7 and 4) |
| `0x802:0x0716` | `RES_0802_0716`, resident DAX block loader, `retf 0x12`, read-only decode in ECL_BLOCK_LOAD.md §3 | `read_block` |
| `0x464:0x00FF` (×2), `0xAF8:0x0919`, `0xAF8:0x084A`, `0x899:0x0059` | notice display and key read; `0x899:0x0059` is the key read named in GAMEPLAY_PATH.md | `error_notice` |
| `0x899:0x0000` | `RES_0899_0000`, the exit routine (GAMEPLAY_PATH.md); PM decode: calls `0xAF8:0x00E9`, which executes DOS `int 0x21` function 0x4C with the code from `[0x441C]` | `exit_program`, required; the model returns `EXITED` at once |
| `0xAF8:0x1685` | Move by shape | `memcpy` of 780 bytes |
| `0xAF8:0x0254` | FreeMem by shape | `release` |
| `0xAF8:0x0DC0` | set membership by shape (GAMEPLAY_PATH.md names the same routine a key-in-set test) with the constant at `+0x0DE8` | `pr_u3bd_slot_in_set` |
| `0x428:0x25` | overlay unit 0x40c entry 1, `+0x003B`, 290 bytes, `retf 4`, four fixup callers, `game_evidenced` | `piece_loaded` |

## Tests (`tests/recovered/test_overlay_u3bd_pieces.c`)

Slot gate over all 256 values (only 1..3 proceed, nothing else touched);
null contracts; each callback missing at the point it is reached with the
earlier effects checked (including the exit callback missing after the
notice); one piece into each of slots 1..3 (callback order
read/release/announce, landing offset, remap against the threshold with
the slot's delta, the slot words, every other byte and word untouched);
threshold above 255 (no remap); delta wrap; id 10 announced as 10 and
stored as 0; three pieces from slot 1 (arguments 51, 52, 53; marks kept on
slots 2 and 3; final write on slot 1); id 0 with two pieces (101, 102;
stored 0); slot 4 reached by 781 bytes from slot 3: copied raw, neither
marked, remapped nor announced; 2,341 bytes from slot 1 fill all four
slots; `BLOCK_BOUNDS` on a short host block. Terminal notice: size 0 with
and without a block (read, notice, exit; nothing copied, released or
written); 3 + 3, 2 + 3 and 3 + 2 pieces/slot exit, 1 + 3 and 2 + 2 do not;
the integer-quotient gate (1,559 bytes from slot 3 passes, 1,560 exits).

Measured: `cc -std=c11 -Wall -Wextra -Wpedantic -Werror` (gcc) builds the
library and test with no diagnostics and prints `unit 0x3bd entry 6 model
tests passed`; CMake build has no warnings; `ctest` passes 29 of 29 targets
on main 8215fc0 (the existing targets plus `poolrad_overlay_u3bd_pieces_tests`).

## Limitations, stated plainly

- Not a full translation: the file-name construction, the DAX block
  loader, the notice display, the program exit, FreeMem and unit 0x40c
  entry 1 are callbacks; what they do is not reproduced. Game-role names
  (pieces, wallsets, pictures) are not asserted by this note; "piece" is
  used only as the 780-byte unit the routine copies.
- The over-read of a trailing partial piece is reported, not reproduced.
- The set constant's membership and the two literal lengths are stated as
  facts; the literals' text is not reproduced.
- Interface: the entry takes `(id, slot)` as bytes, matching PR #97's
  `load_pieces(ctx, id, slot)` callback and the hook's `(value & 0x7F,
  selector)`; connecting either caller to this model is the PM's
  integration step. A host that connects it must treat `EXITED` as the
  program ending, not as a return to the caller.
