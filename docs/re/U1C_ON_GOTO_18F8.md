# Unit 0x1c +0x18F8: ECL ON GOTO (0x25) / ON GOSUB, bounded model

Opcode `0x25` (ON GOTO, [ECL_CONTRACTS.md](ECL_CONTRACTS.md), `2+n` operands) and the
call-frame opcode that shares its handler (`0x26`) dispatch to unit `0x1c`
`+0x18F8..+0x198C`, file `0x263E..0x26D2`, 149 bytes, six bytes of locals, `retf`.
SHA-256 of the span `c51ab20d9189f38a22d05ad50624117f0ff0dc3251b3921fd319e53454f2c8ba`
(GAME.OVR, 206,105 bytes, `872964a3…7928614`). **Classification: bounded model**
composed of existing lane-A pieces (loader, slot value, operand word = strict full-C
entry 9, call frame = bounded heap model); no new hooks. No count, strict full-C or
DOS/native parity is claimed. `deja` found nothing earlier for this handler.

## Original order

| Range | Effect |
| --- | --- |
| `+0x18FE` | `found := 0` |
| `+0x1902..+0x1905` | loader(2), `0x2B:0x2A` |
| `+0x190A..+0x1912` | `sel := low byte of slot value(1)` (`0x2B:0x25`) |
| `+0x1915..+0x191D` | `n := low byte of slot value(2)` |
| `+0x1920` | `dec word [0x49ED]` (undoes the loader's final PC increment) |
| `+0x1924..+0x1928` | loader(n) |
| `+0x192D..+0x1937`, `+0x1939..+0x193D` | loop `i := 0`; exit when `i >= n` or `found` (byte compares) |
| `+0x193F..+0x1945` | `sel = i` (byte compare), else `i += 1` (`+0x1984`) |
| `+0x1947..+0x1970` | latch `[0x6F7F] = 0x25`: `[0x49ED] := word(low[i+1], high[i+1])` via `0x2B:0x4D` |
| `+0x1972..+0x1979` | otherwise: call frame for slot `i+1` via `0x2B:0x75` |
| `+0x197E` | `found := 1`; the loop exits at `+0x193D` |
| `+0x1989..+0x198C` | return |

## Model and bounds

`pr_u1c_op_on_goto` (`overlay_u1c_on_goto_18f8.{h,c}`). Only the first match is acted on;
no match or `n = 0` leaves the PC just past the `n` operands. A literal-tag slot supplies
the stale `high` byte left in the loader state. A slot-value BOUNDS/UNRESOLVED stops
after loader(2) and before the PC decrement; a class-1 slot read runs to completion
and then reports HOOK (PICTURE pattern). The call-frame arm's STACK_FULL (model bound,
64 frames) stops with no frame pushed.

**Count bound (lane-A change).** The saved script's count is 21, above the old
loader limit (`PR_REC_ECL_SLOT_COUNT = 16`, slots 1..15). The engine arrays
`[0x7005+i]`, `[0x7045+i]`, `[0x7085+i]` are `0x40` apart and the string buffers start
at `0x6FC8 + 256k` (`0x70C8` for k = 1), above the last element, so slot indices
below `0x40` do not overlap in the original. The model limit is raised to
`PR_REC_ECL_SLOT_COUNT = 64` (slots 1..63); a count of 64 or more is still a model bound
reported as LOADER_COUNT, mapped to ARGUMENT, with the first loader and the PC
decrement already done. The original has no such check; counts above `0x3F` would overlap
arrays and are not modelled.

Tests (`test_overlay_u1c_on_goto_18f8.c`) are fully synthetic: a grid of selectors
(0..9) and counts (0..7) for both latches against an independent restatement, every
other latch taking the call-frame arm, count 63 working and 64 failing, stack full,
loader failure in each loader, slot value UNRESOLVED in slot 1 and 2, a class-1
selector (HOOK after completion), and the stale high byte.

## Private smoke on the restored slot-A state (documented, not committed)

Hash-pinned slot A (`abca2483…c08f1`), run entry `0x9914`, then search entry `0x9975`
on the retained state with the merged handlers (SAVE, COMPARE, IF, GOTO, AND/OR, EXIT,
PICTURE, PRINT/PRINTCLEAR) and this handler bound; explicit `[0x84E2] = 0`, PICTURE DS
bytes 0, a counting print hook.

* ON GOTO at PC `0x99C9`: selector 0 (the value the search entry stored to the
  window-2 word `0x9800`), count 21, selected index 0, GOTO arm, no call frame; the PC
  becomes the first target `0x9AA5`.
* **Next stop**: dispatch 14, run status UNMODELLED, PC `0x9AA5`, opcode `0x1D`
  (PARTYSTRENGTH, handler `+0x12C6`), previous opcode `0x25`.

This is a supplied-state probe: no DOS reachability, display or one-Up parity.
