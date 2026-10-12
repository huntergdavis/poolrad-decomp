# Unit 0x68 entry 1 (+0x0D6D..+0x10A6): bounded model of the full entry

`src/recovered/overlay_u68_entry1.c` models DOS v1.3 `GAME.OVR` unit `0x68`
public entry 1, the movement-bookkeeping call of the gameplay loop
([GAMEPLAY_PATH.md](GAMEPLAY_PATH.md), `+0x3473..+0x34F2`), which reaches the
merged coordinate writer `+0x073B`
([U68_COORDINATE_WRITER_073B.md](U68_COORDINATE_WRITER_073B.md)) and through it
unit `0x7d` ([U7D_ENTRY1.md](U7D_ENTRY1.md)). Classification: **bounded model
of the whole entry, not strict full C**: eight near callees, the key input,
three far calls and two resident calls are host hooks (several are merged
elsewhere and are wired in by the host), and the five string literals are host
data. Entry 3 and the four runtime string routines are implemented. **No count
or credit is claimed**; the PM decides after parity review. Nothing here says
which branch the live game takes: the model describes every branch and the
tests exercise them with synthetic data.

`deja "Poolrad unit 0x68 entry 1 +0x0D6D Up script gate"` returned the earlier
0x68 notes from this lane; no earlier decode of this entry. Reused with
citation: U68_UP_PREFLIGHT_068D.md and U68_COORDINATE_WRITER_073B.md (caller
context, `[bp-1]` and the `+0x102E` call), U3BD_FIELD_ENTRY3.md (merged entry
3), SLOT_A_UP_ECL_GATE.md and U25_MOVE_ENTRY19.md (the saved slot-A frame:
selector `[0x49FA] = 1`, party `(15, 4)`, facing 6), RESIDENT_ROUTE_290B.md,
RESIDENT_464_BITMAP_RELEASE.md, U68_LIST_SEARCH_05EE.md and
U68_RECORD_CLEAR_0648.md (merged callees; not duplicated), U9E_ENTRY2.md.
Not duplicated here: `+0x04DF`, `+0x05AB`, `+0x05EE`, `+0x0648`, `+0x068D`,
`+0x073B`.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

| Item | Value |
| --- | --- |
| extent | unit `0x68` (file `91,851`) `+0x0D6D..+0x10A6` = file `0x17438..0x17771`, **826 bytes, 350 instructions**, one `retf` at `+0x10A6`; next prologue `+0x10A7` (a 5-byte leaf, entry 0). SHA-256 `a28055269b7373f984337022d88bfdea89993f8eb4a44ee69d6d8c23b2240c55` |
| frame | `sub sp, 0x12C`: `[bp-1]` move flag, `[bp-2]` key-extended byte, `[bp-0x2B]` 40-char Pascal message, `[bp-0x35]` 8-char prompt slot, `[bp-0x12C]` string temporary |
| control flow | **55 basic blocks, 43 jumps, all targets inside the span, no backward jump (no loops)** |
| near calls | `+0x0000` x2, `+0x0236` x3, `+0x0294` x2, `+0x04DF` x1, `+0x05EE` x1, `+0x0648` x2, `+0x073B` x1, `+0x0818` x1 (all `push cs; call`) |
| far calls | `0x3D5:0x2F` (unit `0x3bd` entry 3, merged) x1; `0x3C5:0x2A` (unit `0x3b0`, the key input) x2; `0x3D0:0x2F` (unit `0x3b9`) x1; `0x464:0x2595` (resident, merged bitmap release) x2; `0x8AE:0x00DC` (resident) x1; `0xBA:0x290B` (resident, merged route) x2; runtime `0xAF8:0x0B0B` x8, `0x0B25` x8, `0x0B98` x6, `0x0C10` x2 |
| DS reads | `[0x49FA]`, `[0x6AAD..0x6AAF]`, `[0x6DDE]`, `[0x6DDF]`, `[0x6DE0]`, `[0x2622]`, far `[0x49D6]` (window-1 word `0x592`) |
| DS/ES writes | window-1 word `0x592` := 0 (two sites); `[0x6DDF]` := 0 (one site); `[0x6B00]` := 0xFF, `[0x6B05]` := 0xFF; the locals above. Nothing else is written by this entry itself |
| string pool | `cs:0xD4C, 0xD51, 0xD57, 0xD5E, 0xD64` (Pascal strings of length 4, 5, 6, 5, 8) = the 33 bytes `+0x0D4C..+0x0D6C`, between entry 2's `retf` and this prologue. Their text is not reproduced |

The runtime string routines were read from `START.EXE` (the `0xAF8` mapping is
checked against `0x0B0B` first): `0x0B0B` copies a whole Pascal string and
leaves the destination pointer on the stack; `0x0B98` appends a string to it
and caps the length at 255; `0x0B25` stores the accumulated string into a
destination, truncated to the given maximum (0x28); `0x0C10` compares two
Pascal strings (ZF set = equal, lengths included). Each `s := s + lit` in
this entry is therefore load, append, truncating store; the model's helpers
do exactly that.

## Branch map (byte-verified)

| Block | Offsets | Test | Targets |
| --- | --- | --- | --- |
| gate | `+0x0D6D..+0x0D7F` | `[0x49FA] = 1` | else `+0x1050` |
| gate 2 | `+0x0D82..+0x0D8F` | window-1 word `0x592` < 0xFF (unsigned word) | else `+0x1043` |
| lookup | `+0x0D92..+0x0DB2` | entry 3 result: 1 / 2 / 3 / other | `+0x0DA7` (`[bp-1] := 1`, to `+0x1027`) / `+0x0DB5` / `+0x0EF4` / `+0x1027` |
| block A | `+0x0DB5..+0x0EEA` | result 2: flags `[0x6DDE]`, `[0x6DDF]` and `+0x0236(6)`, `[0x6DE0]` and `+0x05EE(0x1F)`; compare with `cs:0xD5E` | equal: `+0x0000(x, y, facing)` then `+0x1027`; else prompt, keys `B` -> `+0x0294`, `P` -> `+0x04DF`, `K` -> `+0x0648` into `[bp-1]` |
| block B | `+0x0EF4..+0x1026` | result 3: the same chain with `+0x0236(5)` in place of `+0x05EE(0x1F)` | equal: `+0x0000` then `+0x1027`; else prompt, keys `B` -> `+0x0294`, **`P` -> `[0x6DDF] := 0`, no call**, `K` -> `+0x0648` |
| commit | `+0x1027..+0x1041` | `[bp-1] <> 0` | `+0x073B`, else `0x8AE:0x00DC([0x2622])`; then `0xBA:0x290B`; to the tail |
| selector 2..4 | `+0x1050..+0x106E` | `[0x49FA]` in 2..4: word `0x592` < 0xFF | `+0x0818` and `0xBA:0x290B`, else word `0x592` := 0 (`+0x1070`); then the tail. Other selectors go straight to the tail |
| window clear | `+0x1043..+0x104E` | word `0x592` >= 0xFF on the main path | word := 0, to the tail |
| tail | `+0x107B..+0x10A6` | none | `0x3D0:0x2F` on `DS:0x6ABE`; `0x464:0x2595` on `DS:0x6B01` and `DS:0x6B06`; `[0x6B00]`, `[0x6B05]` := 0xFF |

Blocks A and B differ in exactly three places (found by diffing the two
instruction streams): the argument and callee of the third condition, and key
`P`. The message always ends with `lit3` appended, so `s = lit3` (the compare)
holds exactly when nothing was appended before it, or when every earlier part
was cut off by the 40-character cap and `lit3` alone matches.

## The branch the PM asked for: selector 1, entry 3 result 1

Saved slot A has selector 1 and party `(15, 4)` facing 6 (cited notes). With
`[0x49FA] = 1` and word `0x592` below 0xFF, entry 3 returning 1 sets
`[bp-1] := 1` at `+0x0DA7` and jumps to `+0x1027`; there `+0x102B` is not
taken, `push cs; call +0x073B` runs at `+0x102E`, then `+0x103C`
(`0xBA:0x290B`) and the tail. No other call is made on this path: the message
blocks, the key input and the `+0x0000` call are all skipped. Whether the live
game's word `0x592` and entry 3 result are what the saved frame implies at
this call is **not claimed**: the saved word `0x592` is 0 in both owner-held
saves (U68_UP_PREFLIGHT_068D.md), and entry 3's result at the cell the party
stands on after the move depends on the buffer the loader fills.

## Model, hooks and guards

* `pr_u68_e1_hooks` carries one function pointer per unresolved or separately
  merged callee, in call order (see the header). A `NULL` hook does nothing and
  yields 0. The report's `calls[]` records every call in order.
* Entry 3 is called as the merged `pr_u3bd_entry3` on the state's field
  inputs. `PR_U68_E1_ARGUMENT` (nothing written, no hook run): null state,
  window 1 shorter than `0x594`, a prompt literal longer than 8 characters
  (the 9-byte slot), or entry 3's own null-buffer guard.
* String literals are host Pascal strings (`NULL` = empty); the key input hook
  receives the prompt, the 40-char message and the five constants
  `0x0D, 0x0A, 0x0F, 0, 0`; the extended-key byte `[bp-2]` is written by that
  callee and never read by this entry, so it is not modelled.

## Verification

`tests/recovered/test_overlay_u68_entry1.c` (synthetic literals, buffer,
window, every hook recorded; no game data): the selector=1 / result=1 branch
with the exact call order and both DS 0xFF writes; result 0 (no move,
`0x8AE` with `[0x2622]`); 4,096 generated message-block cases (both blocks,
all flag and callee-return combinations, five keys, handler returns 0..3)
against a straight C-string restatement for the message, the compare, the
call order, the key arguments, the key handlers' effect on `[bp-1]`, and the
block-B `P` quirk; the 40-character cap and empty (`NULL`) literals; all 256
selector values with word `0x592` below, at and above the limit (including the
word `0x0100` whose low byte is 0); the ARGUMENT guards; `NULL` hooks; and a
**connected run** where the `+0x073B` hook is the merged
`pr_u68_step_coordinate_writer` on the same party bytes (15 -> 14 with a
synthetic step table, and its `1,1,1` fill is then seen by the next call's
message block). The restatement shares my reading of the decode; byte-level
correspondence is the decode and diff above, not a DOS replay.

## Not covered

The bodies of `+0x0000` (265 bytes), `+0x0236` (94), `+0x0294` (587),
`+0x0818` (369), unit `0x3b0`'s key input, unit `0x3b9` entry via
`0x3D0:0x2F` and resident `0x8AE:0x00DC` remain hooks; `+0x04DF` is p5's.
`+0x0236` (94 bytes, `retf 2`, two DS references) is the smallest unread
callee in this entry.
