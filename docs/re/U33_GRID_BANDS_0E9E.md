# Unit 0x33 +0E9E grid bands

`pr_u33_grid_bands_0e9e` translates the complete DOS v1.3 routine
`GAME.OVR` unit `0x33:+0E9E..+11B3` and composes its five near calls to
the merged unit-0x33 [byte-flags leaf](U33_BYTE_FLAGS_08A4.md) `+08A4`
and its seven far calls to unit `0x9E` public entry 8 (`+0DE3`, the strict
[dice roll](U9E_ENTRY8_ROLL.md)) through the overlay stub `00B0:0048`.
It sweeps the 50 by 25 cell grid and rewrites eligible cells from five
dice bands whose widths the class byte selects. The shared recipe is the
one recorded in [U33_GRID_ROLL_0000.md](U33_GRID_ROLL_0000.md) and the
sibling [U33_GRID_PAIR_0D13.md](U33_GRID_PAIR_0D13.md); the recall
attempt for the cluster found nothing to reuse.

## Exact original, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x9E entries 8 and 9 | `+0DE3..+0E51`, 111 bytes, SHA-256 `9a2681e8b24a9a95838a11f2d7cbddea0c253d29e631db37d25fae017a14693b`; stub paragraph `00B0`, offset `48` |
| SYSTEM generator | START.EXE `0xBE36..0xBEBC` (135 bytes), as the merged [entry-8 roll evidence](U9E_ENTRY8_ROLL.md) pins it |
| Unit 0x33 `+08A4` leaf | `+08A4..+0B96`, the merged `pr_u33_byte_flags_08a4`, reached by `PUSH CS; CALL` with return IP site + 3 |
| Unit 0x33 | GAME file base `0xC5D1`; `+0E9E` is an internal prologue, not a public entry |
| Function | GAME file `0xD46F..0xD784`; **790 bytes, 323 instructions, eighteen conditional branches, seven far calls, five near calls** |
| Function SHA-256 | `3fc5413c258b467850838d7360751ae63be3b0877eaa2409b1cdfd4f840b2ffd` |
| Far calls | `+0F45` (1, `0FFh`), `+0F5E` (1, 4), `+1026` (1, 3), `+107B` (1, 4), `+10D9` (1, `0Ah`), `+1150` (1, 4) and `+116E` (1, 3), each `LCALL 00B0:0048` with a fixup-backed segment word |
| Near calls | `+0EB9`, `+0ECB`, `+0ED9`, `+0EEB` and `+115F` `CALL +08A4`, each preceded by `PUSH CS` |
| Exit | `RETF` at `+11B3`; exact end `+11B4` exclusive |

Every branch target lies inside the 790-byte body. Decoding every
unit-0x33 routine from its own prologue finds exactly one aligned
`PUSH CS; CALL +0E9E`: `+1221` inside the `+11B4` driver (third of its
four generator calls), itself reached from the mode dispatcher at
`+1244`. The body reads the far grid `DS:6716/6718` with the 50-byte row
stride and `+7` cell byte and the four-byte-stride DS table at `2747`
exactly as `+0000` and `+0D13` do. No gameplay meaning is inferred for
the written values; "grid bands" names the banded roll.

## Complete ABI and ordered effects

The API enters after the far CALL with `SS:[SP]` return IP and `[SP+2]`
return CS and no arguments; `RETF` leaves SP four bytes above entry. The
routine pushes BP and reserves eight local bytes: band widths `[BP-1]`
(0), `[BP-2]` (6), `[BP-3]` (`0Fh`), `[BP-4]` (`28h`) and `[BP-5]` (0),
roll `[BP-6]`, column `[BP-7]` and row `[BP-8]`.

Four `+08A4` calls adjust the widths: AL bit `02h` sets `[BP-3]` to
`2Dh` and `[BP-4]` to `0Ah`; bit `04h` sets `[BP-3]` to `19h`; bit `40h`
sets `[BP-4]` to 5 and `[BP-5]` to `1Eh`; bit `80h` sets `[BP-1]` to
`0Fh`, `[BP-4]` to `19h` and `[BP-3]` to `0Ah`. For column 0..`31h` and
row 0..`18h` a cell mapping through `DS:2747 + 4 * cell` to `16h` takes a
(1, `0FFh`) roll `r` and the first band whose cumulative width holds it:

| Band (signed 16-bit compare) | Effect on the cell |
| --- | --- |
| `r <= [BP-1]` | (1, 4): a 4 with row above 0 and an eligible cell above writes `41h` above and `40h` here; below 4 writes roll + `3Ch` |
| `r <= [BP-1] + [BP-2]` | (1, 3) + `36h` |
| `r <= [BP-1] + [BP-2] + [BP-3]` | (1, 4) + `29h` |
| `r <= [BP-5] + [BP-3] + [BP-1] + [BP-2]` | (1, `0Ah`): (roll - 1) `IDIV` 3 plus 1, then + `38h` |
| `r <= [BP-4] + [BP-5] + [BP-3] + [BP-1] + [BP-2]` | (1, 4); a 4 is rerolled by (1, 3) unless a fifth `+08A4` call sets bit `40h`; + `2Dh` |

A roll above every band leaves the cell. The row compare with `18h` and
the column compare with `31h` reread their live locals; the final flags
are the column compare's. AX, BX, CX, DX, SI, ES and DI return as the last
address computation, lookup, roll or leaf left them. Every input returns.

The far and near calls, the loaded-state stub checks, the `CODE`,
`CALLEE`, `TRANSFER` and `ARGUMENT` statuses, the wrap rules and the
folding of flag writes that a later instruction always overwrites are
exactly as documented for [`+0D13`](U33_GRID_PAIR_0D13.md).

## Validation and parity review

The focused C11 test (`-Wall -Wextra -Wpedantic -Werror`, also run under
ASan/UBSan) finds a residue-independent class byte whose leaf result sets
bit `80h` (first band width `0Fh`), predicts an all-eligible grid cell by
cell from the generator formula in column-major order through every
band, checks a grid with no eligible cell leaves the seed and grid
untouched, a stub that is not a far jump reports `CODE`, and the host
guards leave the CPU words unchanged.

The original-byte oracle passes **147 returning cases** with the
same case families as `+0D13` (72 random, 52 class bytes, no eligible
cell, all eligible, grid, window, stack, SS and DS boundary placements, a
random background and preserved DF), each comparing the entire 1 MiB RAM
and all 14 CPU words with no flag mask. All 323 instruction starts and all 36 conditional outcomes are reached, so no outcome is declared infeasible.

The DOSBox-X normal-core/386 synthetic oracle passes **70
cases** for this body in the shared cluster run with the same COM layout,
captured state and interrupt isolation as `+0D13`; the result SHA-256 is
`a7cafff592f0c7e3a41d4ca004112e22a7660fe5f6c155edd53b1305da894f33` with COM segment `0814`. The build, test and
audit commands are listed in [U33_GRID_PAIR_0D13.md](U33_GRID_PAIR_0D13.md).

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x33` `+0E9E..+11B3` with `+08A4` composed at five sites and entry 8 at seven |
| `source_candidates` | `src/recovered/overlay_u33_grid_bands_0e9e.c` |
| `signals` | Exact hash/CFG, one aligned caller inside the game-evidenced driver, the documented grid and table globals, whole-RAM/CPU differential with full instruction coverage and every roll-reachable branch outcome, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Every conditional outcome reached; loaded-state stubs only; natural saved-game reach unmeasured |

No classifier or count ledger is changed. Identity and strict credit
remain the PM's review decision.
