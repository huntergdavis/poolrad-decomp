# Unit 0x68 internal +0x0818: tile step (bounded model)

`src/recovered/overlay_u68_tile_step_0818.c` models DOS v1.3 `GAME.OVR` unit
`0x68` internal routine `+0x0818..+0x0979`, the routine entry 1 calls when
`[0x49FA]` is 2, 3 or 4 and window-1 word `0x592` is below 0xFF
([U68_ENTRY1.md](U68_ENTRY1.md), `+0x1050..+0x106E`). It computes a new
position from window-0 words `0x186`/`0x188` and the direction tables, clamps
it, looks up a tile byte in a DS table, and either returns, shows a message, or
commits the position and advances the clock. Classification: **bounded model,
not strict full C**: the resident message call `0x709:0x0D68` and unit `0x7d`
entry 1 are hooks (the latter is merged as overlay_u7d_entry1.h and a test
wires it in); the DS image, window 0 and the string constant are host state.
**No count or credit is claimed**, and no saved-slot claim: with selector 1 this
routine is not reached.

`deja "Poolrad unit 0x68 +0818 selector 2..4 helper"` found no earlier trace.
Reused and re-checked: U68_ENTRY1.md (the single call site `+0x1066`),
U7D_ENTRY1.md (`0x8C:0x25` and its `(12, 3)` callers, window-0 words
`+0x18C..`), U68_UP_PREFLIGHT_068D.md (the direction tables `0x2736`/`0x273F`),
U68_COORDINATE_WRITER_073B.md (the `cs:0x07F4` string that this routine uses).

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

| Item | Value |
| --- | --- |
| code extent | unit `0x68` (file `91,851`) `+0x0818..+0x0979` = file `0x16EE3..0x17044`, **354 bytes, 144 instructions**, `retf` at `+0x0979`. SHA-256 `ae61490d647b768a8ebd1036eefded80e6868aee6a1d29c5b2043672e6611456` |
| trailing data | `+0x097A..+0x0988`, 15 bytes = file `0x17045..0x17053`, SHA-256 `ff42ad73d0b6fdc03f941f6565841ba72699ac4a166e86beba00075ce6ebfea4`: Pascal string constants (a pad byte, then two length-prefixed strings of 8 and 4 characters) that **entry 2** reaches with `cs:0x097A`, `cs:0x097B` and `cs:0x0984` (six references inside `+0x0989..`); this routine never references them. 354 + 15 = 369, the ledger bound; the ledger's 154 instructions count the data decoded as code. The text is not reproduced |
| frame | `sub sp, 0x28`: `[bp-1]` column byte, `[bp-2]` x, `[bp-3]` y, `[bp-0x27..-0x4]` a 36-byte string slot; no arguments, no result |
| control flow | 26 jumps, all internal, all forward |
| calls | runtime `0xAF8:0x0B0B` x1 (string copy), resident `0x709:0x0D68` x1, far `0x8C:0x25` (unit `0x7d` entry 1) x2 |
| near callers | exactly one: entry 1 at `+0x1066` (a scan of every `push cs; call rel16` with target `+0x0818`) |
| DS reads | `[0x6AAF]`, `[0x49FA]`, `[di + 0x2736]`, `[di + 0x273F]` (`di` = facing), `[di + 0x3C76]` (`di` = selector), `[di + 0x3648]` (the tile), far `[0x49D2]` |
| memory | window 0 words `0x186`, `0x188` read; `0x344` read; `0x186` and `0x188` written (two sites each). No DS write |

## Paths

| Offsets | Effect |
| --- | --- |
| `+0x081E..+0x083E` | `x := low byte of (word[0x186] + cbw(DS[0x2736 + facing]))` (a 32-bit add; only AL kept) |
| `+0x0841..+0x0861` | `y := low byte of (word[0x188] + cbw(DS[0x273F + facing]))` |
| `+0x0864..+0x0888` | signed-byte clamps: `x` to 0..0x0F, `y` to 0..0x23 |
| `+0x088C..+0x08B7` | `col := low byte of (DS[0x3C76 + selector] + x)`; `tile := DS[0x3648 + (y * 0x2C + cbw(col)) mod 65536]` (a 44-column row stride) |
| `+0x08BB..+0x08F7` | tile in {`0x4E`, `0x7E..0x80`, `0x82..0x84`, `0xA8..0xAC`, `0xAE..0xB3`, `0xBC`, `0xDD..0xDE`, `0xE1..0xE9`, `0xF5`} -> return (nothing changed, no call) |
| `+0x08F9..+0x0903` | tile in {`0x66..0x6C`, `0x72`} -> `+0x0905`; any other tile -> the move at `+0x0951` |
| `+0x0905..+0x0928` | word `0x344` = 0: copy the Pascal string `cs:0x07F4` to the local slot (the string is 36 bytes, so at most 35 characters), call `0x709:0x0D68` with the slot and the byte `0x0E`; return, no move |
| `+0x092A..+0x094F`, `+0x0951..+0x0971` | words `0x186 := x`, `0x188 := y` (`cbw`, high bytes 0), then `0x8C:0x25` with `(0x0C, 0x03)` |
| `+0x0976..+0x0979` | `mov sp, bp; pop bp; retf` |

So a "special" tile moves only when word `0x344` is non-zero; every other
non-blocked tile moves. The clock call is the `(12, 3)` form listed in
U7D_ENTRY1.md: 12 units of window-0 word `+0x192`, with the unit `0x7d`
carry normalizer. What the tile values, the 44-column map, the selector table
and word `0x344` mean is **not asserted**. The selector table at `0x3C76 +
selector` lies at the end of the tile rows (row 35 ends at `0x3C77`); only
selectors 2..4 reach this routine through entry 1, but the model takes any
selector byte.

## Host representation and guards

* `ds` is a host image of the data segment from offset 0 (`ds_size` bytes);
  the model reads only `0x3C76 + selector` and the computed tile address, both
  with the original's 16-bit wrap, and returns `PR_U68_0818_ARGUMENT` for an
  address outside the image. Window 0 needs 0x346 bytes; the direction tables
  `facing + 10`. All checked before any write or hook call.
* The string `cs:0x07F4` is read by the original only on the reached branch
  (special tile and word `0x344` = 0, `+0x0916..+0x091B`). The model therefore
  requires `message` non-NULL with at most 35 characters **only there**, and
  returns `PR_U68_0818_MESSAGE` (no hook, no write) otherwise; on every other
  path the message pointer is ignored, whatever it holds.
* Every public name in this header carries the `PR_U68_0818_` / `pr_u68_0818_`
  prefix. The test includes the coordinate-writer header in the same
  translation unit as a dual-header compile check: before the rename this
  header's status constants collided with that header's `PR_U68_STEP_OK` and
  `PR_U68_STEP_ARGUMENT`.
* `PR_U68_0818_HOOK`: the reached hook is `NULL`; unlike the adjacent `+0x04DF`
  model this is checked **before** the position words are written, so no effect
  is retained on that status.
* The message hook receives the host string pointer (the original copies it to
  a local first) and the colour byte.

## Verification

`tests/recovered/test_overlay_u68_tile_step_0818.c`: 6 facings x 5 selectors x
10 x-words x 9 y-words (the low byte only is used; values include 0xFFFF,
0x1005, 0x8000) x every tile value 0..255 (the tile placed at the address a
wide-integer restatement computes), with four values of word `0x344` (including
0x0100, whose low byte is 0, and 0xFFFF): the clamped position, column, tile
address, the blocked and special sets (independent interval lists), the message
and move outcomes, the hook arguments, that the position words are written
before the clock call with zero high bytes, and that no other window byte
changes (about 0.77 million cases, 0.4 s). Pinned clamp edges with a zero step
table; the 35- and 36-character message boundary and a NULL message on the reached
branch (`PR_U68_0818_MESSAGE`), with the same invalid messages ignored on the
move and blocked paths; guards (missing hooks only when reached and with no
write, short window, short table, DS image too small for either read, null); and a connected run of the merged
`pr_u7d_entry1` with `(0x0C, 0x03)` (`25 + 12` over a limit of 30: word 7 with one
carry). Synthetic data only; clean under AddressSanitizer and
UndefinedBehaviorSanitizer; strict `-Wall -Wextra -Wpedantic -Werror` full
build. The restatement shares my reading of the decode; the correspondence is
the tables above, not a DOS replay.
