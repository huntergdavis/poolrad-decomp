# Unit 0x33 +11B4 grid driver

`pr_u33_grid_driver_11b4` translates the complete DOS v1.3 routine
`GAME.OVR` unit `0x33:+11B4..+122B`, folds its far call to the SYSTEM
`FillChar` at `0AF8:16A8`, and composes its four near calls to the
unit-0x33 generators [`+0BDE`](U33_GRID_DIAGONAL_0BDE.md),
[`+0D13`](U33_GRID_PAIR_0D13.md), [`+0E9E`](U33_GRID_BANDS_0E9E.md) and
[`+0000`](U33_GRID_ROLL_0000.md) on the same RAM and CPU words. It fills
the grid, derives the class byte from the window and the mode, and runs
the four generators in order.

## Exact original, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| SYSTEM `FillChar` | START.EXE `0xC31E..0xC331` (20 bytes), loaded at `0AF8:16A8`; SHA-256 `f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7`; `MOV BX,SP; LES DI,SS:[BX+8]; MOV CX,SS:[BX+6]; MOV AL,SS:[BX+4]; CLD; REP STOSB; RETF 8` |
| Unit 0x9E entries 8 and 9, SYSTEM generator, `+08A4` leaf | As pinned in the generator documents above |
| Unit 0x33 | GAME file base `0xC5D1`; `+11B4` is an internal prologue, not a public entry |
| Function | GAME file `0xD785..0xD7FC`; **120 bytes, 46 instructions, two conditional branches, one far call, four near calls** |
| Function SHA-256 | `b3d4afee1c3eef42c8abc434d092ce87528fcf9ed3854a865366ae889e94ad7d` |
| Far call | `+11CB` `LCALL 0AF8:16A8` with a fixup-backed segment word |
| Near calls | `+1219` `+0BDE`, `+121D` `+0D13`, `+1221` `+0E9E` and `+1225` `+0000`, each preceded by `PUSH CS` |
| Exit | `RETF` at `+122B`; exact end `+122C` exclusive |

Both branch targets lie inside the 120-byte body. Decoding every
unit-0x33 routine from its own prologue finds exactly one aligned
`PUSH CS; CALL +11B4`: `+12DA` inside the routine at `+1244`, which tests
`DS:49FA` against 1 and calls `+0810` for that mode and `+11B4` for every
other. The body reads the far grid `DS:6716/6718`, the far window
`DS:49D2/49D4`, the mode byte `DS:49FA`, the `2Ch`-stride class table at
`DS:3648` and writes the class byte `DS:465A` that the generators' `+08A4`
leaf reads. No gameplay meaning is inferred; "grid driver" names the
sequence.

## Complete ABI and ordered effects

The API enters after the far CALL with `SS:[SP]` return IP and `[SP+2]`
return CS and no arguments; `RETF` leaves SP four bytes above entry. The
routine pushes BP and reserves four local bytes: first window byte
`[BP-1]` and second window byte `[BP-2]`.

It pushes the far grid pointer plus 7, the count `4E2h` (1,250 cells) and
the byte `17h` (in AX, so the pushed word carries AH `04h`) and far-calls
`FillChar`, which fills the 1,250 cell bytes, clears DF, leaves BX as the
callee's SP, CX zero, ES:DI past the fill and AL `17h`, and discards the
eight argument bytes. `[BP-1]` and `[BP-2]` take the window bytes at
`+186h` and `+188h`; a mode of 3 adds `0Dh` and a mode of 4 adds `1Ah` to
the first. The class byte is `DS:[3648h + 2Ch * second + first]` with both
bytes sign-extended (`CBW`, `MUL` by `2Ch`), and is stored at `DS:465A`.
The four generators then run in order through `PUSH CS; CALL`, each
returning to the pushed frame; `MOV SP,BP; POP BP; RETF` ends. The final
registers and flags are the last generator's (`+0000`, whose final flags
are its column compare's).

The `FillChar` site is checked for its original five bytes (`CODE`
otherwise) and entered at the relocated segment word read from the site,
exactly as the stubbed dice calls are; a generator's `ARGUMENT`, `CODE`
or `CALLEE` status is returned unchanged, any other failure or a frame
that does not return reports `TRANSFER`, and `*chain_status` carries the
dice chain's own status. `MUL` sets CF/OF from the high word and, as the
Unicorn/QEMU oracle does for the 8086's undefined bits, SF/ZF/PF from the
low word with AF clear. Offsets wrap at 16 bits; words, LES operands and
return frames are physically contiguous; physical addresses wrap at
1 MiB. Null RAM/CPU or RAM that is not exactly 1 MiB is rejected before
mutation.

## Validation and parity review

The focused C11 test (`-Wall -Wextra -Wpedantic -Werror`, also run under
ASan/UBSan) runs the driver with no eligible cell so the four generators
make their class checks and gate roll without writing: the grid is the
`FillChar` result with the seven bytes before and the byte after
untouched, the class byte is the table lookup for mode 3 and for mode 4,
the frame returns to the pushed CS:IP; a corrupted `FillChar` site reports
`CODE` before any fill; the host guards leave the CPU words unchanged.

The original-byte oracle passes **95 returning cases**, each
running the whole driver with the original bytes of all four generators,
the leaf, the dice chain, the SYSTEM generator and `FillChar`, and
comparing the entire 1 MiB RAM and all 14 CPU words with no flag mask: 36
random grids, tables, class tables, window words and bytes, modes, seeds,
flags and ES; 37 class bytes 0..255 in steps of 7; no eligible cell; the
grid across A20, at offset wrap, under the stack and over the seed; the
window under the stack and over the class byte; eleven stack offsets at
both segment ends on a zero background; SS and DS across A20; a random
background; and preserved DF. All 46 instruction starts and all 4 conditional outcomes are reached, so no outcome is declared infeasible. The stack offset `0Ah` stands
in for 8: at 8 the first generator call pushes CS at `SS:0000` and the
return IP at `SS:FFFE`, so the callee's `RETF` reads CS physically beyond
the segment end under the contiguous-frame contract that the emulator and
the model share, and the original leaves the code; the model reports
`TRANSFER` for that layout, which is not a loaded-state frame. Offset
`0Ah` still wraps `FillChar`'s argument reads around the segment end.

The DOSBox-X normal-core/386 synthetic oracle passes **40
cases** for this body in the shared cluster run with the COM layout,
captured state and interrupt isolation described in
[U33_GRID_DIAGONAL_0BDE.md](U33_GRID_DIAGONAL_0BDE.md); the `FillChar`
bytes are copied to the absolute `0AF8:16A8` beside the generator so the
original site's segment word is the loaded one. The result SHA-256 is
`2bfa49e1693778bdd4253d6862d0b0a5917474fe7dd0de4f00633f7ee9c9d8a6` with COM segment `0814`. The build, test and
audit commands are listed in that document.

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x33` `+11B4..+122B` with `FillChar` folded and four generators composed |
| `source_candidates` | `src/recovered/overlay_u33_grid_driver_11b4.c` |
| `signals` | Exact hash/CFG, one aligned caller in the mode dispatcher, the documented grid, window, mode, class-table and class-byte globals, whole-RAM/CPU differential through the whole generator sequence with full instruction and branch-outcome coverage, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Every conditional outcome reached; `FillChar` and `+0B97` folded pending their own models; natural saved-game reach of `+1244` unmeasured |

No classifier or count ledger is changed. Identity and strict credit
remain the PM's review decision.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_0033_11B4 --require-current-main
```
