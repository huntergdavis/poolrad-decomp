# Unit 0x68 internal +0x0294: party roll gate (bounded model)

`src/recovered/overlay_u68_roll_gate_0294.c` models DOS v1.3 `GAME.OVR` unit
`0x68` internal routine `+0x0294..+0x04DE`, the routine entry 1 calls for key
`B` ([U68_ENTRY1.md](U68_ENTRY1.md)); its result AL is stored in entry 1's
`[bp-1]`, the move flag. It is the sibling of p5's `+0x04DF` model
([U68_MOVEMENT_FLAG_04DF.md](U68_MOVEMENT_FLAG_04DF.md)): same party list, same
roll call, same two map-updater calls. Classification: **bounded model, not
strict full C**. The roll (`0xB0:0x0048`, unit `0x9e` entry 8) and the map
updater are explicit hooks; entry 3 is the merged translation; far pointers are
host views. **No count or credit is claimed**; the PM decides after review.

`deja "Poolrad unit 0x68 +0294 587 bytes"` found no earlier trace. Reused and
re-checked: U68_MOVEMENT_FLAG_04DF.md and its source (hook conventions: the roll
arguments, the `0xFFFF` link guard, the cycle guard), U68_MAP_UPDATE_0109.md and
`pr_u68_map_update` (the callee behind the two marks), U3BD_FIELD_ENTRY3.md,
U68_ENTRY1.md (call sites `+0x0ECA`, `+0x1009`), U9E_ENTRY2.md (unit `0x9e`
stub mapping: `0xB0:0x48` = entry 8).

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

| Item | Value |
| --- | --- |
| extent | unit `0x68` (file `91,851`) `+0x0294..+0x04DE` = file `0x1695F..0x16BA9`, **587 bytes, 247 instructions**, `retf` at `+0x04DE` (no argument); next prologue `+0x04DF`, so the ledger bound is exact and there is **no trailing data**. SHA-256 `8a2393e6e0632118ba5094510ee1d8aa5a0a3a2c84ef2eb1ad27a0f85844ef48` |
| frame | `sub sp, 0xA`: `[bp-6]` found, `[bp-5]:[bp-3]` list cursor, `[bp-0xA]:[bp-8]` the node of this iteration; result AL = found (also stored to `[bp-1]`) |
| control flow | 57 jumps, all internal; one loop (`+0x047B jmp +0x02AB`) |
| calls | far `0x3D5:0x2F` (unit `0x3bd` entry 3, merged) x1, inside the loop; far `0xB0:0x48` (unit `0x9e` entry 8, the roll) x12 sites; near `+0x0109` (map updater) x2 after the loop. No runtime or resident call |
| near callers | exactly two, both in entry 1: `+0x0ECA` and `+0x1009` (a scan of every `push cs; call rel16` with target `+0x0294`) |
| DS reads | `[0x5D96]` (party head), `[0x6AAD..0x6AAF]`, direction tables `[di + 0x2736]` and `[di + 0x273F]` (`di` = facing, on a hit only) |
| DS writes | `[0x6DDE] := 0` at two sites (`+0x0328`, `+0x03A5`); nothing else |
| memory reads | node byte `+0x10` every node; byte `+0x16` **only on the type `0x12` branches** (`+0x02EB` with entry 3 = 3, `+0x0410` otherwise; never read for another type); the next far pointer at `+0x104`/`+0x106` |

## Rules (the byte-verified compare chains)

For each node, entry 3 is called with the party bytes (the arguments never
change between iterations), then the branch is chosen by entry 3's result and
the node's byte `+0x10` (`type`), plus byte `+0x16` (`value`), which is read only for type `0x12` (the model reads it only then).
Rolls are `0xB0:0x48` with the byte arguments pushed as (1, sides). The compare
on AL is unsigned; the two `== 1` forms use `cmp al, 1; jne`, so a roll of 0 does
**not** count there, while every `<= n` form (`ja` skips) does.

| Entry 3 | `type` | `value` | Effect | Offsets |
| --- | --- | --- | --- | --- |
| = 3 | 0x12 | 0x5B..0x63 | roll 1d6 == 1 -> found | `+0x02E3..+0x030D` |
| = 3 | 0x12 | 0x64 | roll 1d6 <= 2 -> found | `+0x030F..+0x0326` |
| = 3 | 0x12 | other | `[0x6DDE] := 0`, no roll | `+0x0328` |
| = 3 | 0x13, 0x14 | | roll 1d6 <= 3 | `+0x032F..+0x034A` |
| = 3 | 0x15, 0x16 | | roll 1d6 <= 4 | `+0x034C..+0x0367` |
| = 3 | 0x17 | | roll 1d6 <= 5 | `+0x0369..+0x0380` |
| = 3 | 0x18 | | roll **1d8** <= 7 | `+0x0382..+0x0399` |
| = 3 | 0x19 | | found, no roll | `+0x039B..+0x03A3` |
| = 3 | any other | | `[0x6DDE] := 0`, no roll | `+0x03A5` |
| <> 3 | 3..7 | | roll 1d6 == 1 | `+0x03B4..+0x03CF` |
| <> 3 | 8..0xF | | roll 1d6 <= 2 | `+0x03D2..+0x03ED` |
| <> 3 | 0x10, 0x11 | | roll 1d6 <= 3 | `+0x03EF..+0x040A` |
| <> 3 | 0x12 | 0..0x32 / 0x33..0x63 / 0x64 | roll 1d6 <= 3 / <= 4 / <= 5 | `+0x0410..+0x0464` |
| <> 3 | 0x12 other value, or any other type | | nothing | |

After a hit the loop still reads the node's next pointer (`+0x0468..+0x0478`),
then ends at the next test. When `found`: the map updater `+0x0109` runs with
`(x, y, facing)` and then with `(x + dx[facing], y + dy[facing], (facing + 4)
mod 8)`; the 16-bit sums are pushed as words and the callee reads only their low
bytes. In this tree `+0x0109` (U68_MAP_UPDATE_0109.md) range-checks x and y and
*sets* the selected 2-bit field to 1 (`(byte & keep) | set`); `+0x0000`
([U68_FIELD_CLEAR_0000.md](U68_FIELD_CLEAR_0000.md)) is the routine that clears
it. What the node bytes, the type values and the dice mean is **not asserted**.

## Host representation and guards

* Far pointers, views, cycle and capacity guards follow
  overlay_u68_movement_flag.h exactly (`pr_u68_far`, `pr_u68_segment_view`,
  64 KiB views, a link word at `0xFFFF` rejected, scratch for the visited
  nodes). The roll hook takes `(range, count)` like p5's, with `count` = 1.
* `PR_U68_ROLL_ARGUMENT`: null state/result, a missing or short view for a
  reached node, a link word at `0xFFFF`, a short direction table when reached
  (after the first mark, as in p5's model), or entry 3's null-buffer guard (on
  the first node, before any roll). `PR_U68_ROLL_HOOK`: a reached hook is NULL
  (effects before it are retained). An empty list never calls entry 3.

## Verification

`tests/recovered/test_overlay_u68_roll_gate_0294.c`: every combination of 18
type values and 9 byte values, with entry 3 result 3 and not 3 and every roll
value 0..9, against a range-table restatement (different representation from
the model's compare chain): found, the number and arguments of the rolls, the
`[0x6DDE]` clears, entry 3's result, and the two mark calls' exact arguments;
3,000 generated lists of 0..4 nodes (the first hit stops the rolls; clears
before it are counted); a connected run in which the two marks drive the merged
`pr_u68_map_update` on the same 64 KiB map segment entry 3 reads (the fields at
`(15, 4)` and the neighbour set exactly as expected); and the guards (missing
hooks only when reached, cycle, capacity, short or missing view, link at
`0xFFFF`, short table, entry-3 buffer, empty list). Synthetic data only; clean
under AddressSanitizer and UndefinedBehaviorSanitizer; strict
`-Wall -Wextra -Wpedantic -Werror` full build. The restatement shares my reading
of the decode; the correspondence is the tables above, not a DOS replay.
