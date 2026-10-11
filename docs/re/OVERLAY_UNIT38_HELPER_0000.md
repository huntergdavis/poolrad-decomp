# Full C translations: unit 0x38 private helper +0x0000 and entries 93, 94, 96 (issue #76)

`src/recovered/overlay_unit38_helper.c` translates the DOS v1.3 `GAME.OVR`
unit `0x38` private helper at `+0x0000` (the first bytes of the unit, not a
public entry) and the three public dispatch entries whose only action is to
call it: 93 (`+0x29CB`), 94 (`+0x29D9`) and 96 (`+0x2A11`). The helper's
call ABI is proved from all twenty near-call sites in the unit. Per the
issue, these stay out of the game-specific full count until role and parity
are reviewed: the helper's game role is not established (classification
grade B, `game_or_other_library_unresolved`), only its complete behavior.

`deja "unit 0x38 private helper +0x0000 6817 6818 entries 93 94 96"`
matched only this worker's PR #63/#69 sessions; nothing prior covered the
helper. The dispatcher and argument-frame findings of
[OVERLAY_UNIT38_EFFECT_ENTRIES.md](OVERLAY_UNIT38_EFFECT_ENTRIES.md) are
reused for the three public entries and re-checked here (installer stores
below). ECL issue #75 and resident files are untouched.

## Provenance (ignored local input; hashes only)

`GAME.OVR`, 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`; unit
`0x38` at file offset 60,507 (`0x0EC5B`), stub segment `0x41`, public
entry *j* at stub `0x41:(0x20 + 5·j)`. Capstone 5.0.7, 16-bit mode, decoded
from each start offset to the next ledger prologue; every jump target lies
inside its routine; no trailing bytes after any of the four returns.

| Routine | Unit offset | File offset | Bytes | Insns | Return | Item | Installer store | SHA-256 of the routine's bytes |
| --- | --- | --- | ---: | ---: | --- | ---: | --- | --- |
| private helper | +0x0000 | 0x0EC5B | 31 | 11 | `retf 2` | (none) | (not a table entry) | `f62ad6b7474059693853b623f564cfd5c9298cd50e4628732b49b97eb6265240` |
| entry 93 | +0x29CB | 0x11626 | 14 | 8 | `retf 0xA` | 109 | `[0x69DC]` at +0x3714 | `43c37b0e4abefab2d5e537d4a931904ed923a5eb619915cbe25433b166390f26` |
| entry 94 | +0x29D9 | 0x11634 | 23 | 12 | `retf 0xA` | 110 | `[0x69E0]` at +0x3721 | `4428de57a40652837b6b3337f968b56d6fd797c613cc2ff99ec6ab92af12b636` |
| entry 96 | +0x2A11 | 0x1166C | 23 | 12 | `retf 0xA` | 112 | `[0x69E8]` at +0x373B | `0782a6473c40d4134208a3d895b145b1c30f4225c5da76588f15eb47d906e84d` |

The public classification lists `far_in` 0 for all four; the helper has no
public entry slot and no fixup-confirmed far caller, so it is reached only
by near calls from inside the unit.

## The helper's call ABI, from every call site

All twenty near calls to `+0x0000` in unit 0x38 (ledger `near_called` 20)
have the same three-instruction shape immediately before the call:
`mov al, imm8; push ax; push cs; call 0x0000`. Pushing CS and then
near-calling builds the frame of a far call, and the helper returns with
`retf 2`, which pops IP, CS and the one argument word. Inside the helper,
after `push bp; mov bp, sp`, the argument word is at `[bp+6]` and only its
low byte is read; the high byte is whatever the caller's AH held.

| Caller (unit 0x38) | Entry | Sites | Keys pushed |
| --- | ---: | --- | --- |
| +0x09C0 | 24 | +0x09EF | 0 |
| +0x1490 | 50 | +0x14E2 | 0 |
| +0x2556 | 76 | +0x2567, +0x2599 | 0, 0 |
| +0x2915 | (private, `retf 2`) | +0x296D | 0 |
| +0x2992 | 91 | +0x29A8, +0x29AF | 0x35, 0x0B |
| +0x29B6 | 92 | +0x29BD, +0x29C4 | 0x0B, 0x35 |
| +0x29CB | 93 | +0x29D2 | 0x34 |
| +0x29D9 | 94 | +0x29E9 | 0 |
| +0x29F0 | 95 | +0x29F7, +0x29FE | 0x37, 0x34 |
| +0x2A11 | 96 | +0x2A21 | 0 |
| +0x2DE4 | 108 | +0x2DFA, +0x2E01 | 0x0B, 0x35 |
| +0x2E08 | 109 | +0x2E0F, +0x2E16, +0x2E1D, +0x2E24 | 0x0B, 0x35, 0x34, 0x37 |

Twelve routines, twenty sites, five distinct keys (0, 0x0B, 0x34, 0x35,
0x37). Every key is an immediate; no site passes a computed value.

## Instructions, branches, side effects

**Helper +0x0000.** `push bp; mov bp,sp; cmp byte [bp+6],0; je clear; mov
al,[0x6817]; cmp al,[bp+6]; jne done; clear: mov byte [0x6818],0; mov byte
[0x6817],0; done: pop bp; retf 2`. Edges +0x0007 `je`→+0x0011, +0x000F
`jne`→+0x001B. Three paths: key 0 (clear), key equal to `[0x6817]` (clear),
otherwise no change. Side effects on the clear paths: `[0x6818] = 0` then
`[0x6817] = 0`. By use pattern `[0x6817]` is a latched key that the helper
compares and consumes, and key 0 is an unconditional reset; that is a
reading, not an asserted role.

**Entry 93 +0x29CB.** `mov al,0x34; push ax; push cs; call +0x0000`, then
`pop bp; retf 0xA`. One path. Effect: the helper with key 0x34, so the two
bytes are cleared exactly when `[0x6817] == 0x34`.

**Entries 94 +0x29D9 and 96 +0x2A11.** `mov al,[0x6819]; and al,mask; or
al,al; je done; mov al,0; push ax; push cs; call +0x0000; done: pop bp; retf
0xA`, with mask 2 (94, edge +0x29E3 `je`→+0x29EC) and mask 1 (96, edge
+0x2A1B `je`→+0x2A24). Two paths each. When the bit is set the helper is
called with key 0, which always clears `[0x6818]` and `[0x6817]`. Bit 0 of
`[0x6819]` is the bit the merged entries 18, 77 and 97 test; bit 1 is the
bit entries 10 and 102 test. The five discarded dispatch words are never
read by any of the three entries.

**Writers of the touched bytes.** `[0x6817]` is written only by this helper
(cleared) and by unit 0x9e `+0x1656` (entry 20, at +0x1672, from its
`[bp+0x14]` argument byte); it is read by the helper, by unit 0x38 `+0x2915`
(`cmp` with 0 at +0x2947) and by unit 0x9e entry 20 (`cmp` with 0 at
+0x1682). `[0x6818]` has 33 references across unit 0x38 (merged entries
halve, add to and clear it). `[0x6819]` is read here only.

## C contract and tests

`pr_u38h_state` holds the three bytes. `pr_u38_helper_0000` takes the key
byte directly (the only part of the argument word the original reads).
`paths` reports `PR_U38_PATH_EFFECT` when the clear ran; entries 94 and 96
add `PR_U38_PATH_SECOND` when the call was made (always together with the
clear, since key 0 clears). Null state returns `PR_U38_ARGUMENT`.

`tests/recovered/test_overlay_unit38_helper.c`: every key 0..255 against 86
values of `[0x6817]` (clear iff key is 0 or equals the latch; `[0x6819]`
untouched); key 0 against a nonzero latch; a consumed latch no longer
matches; entry 93 over all 256 latch values (only 0x34 clears, flag byte
ignored); entries 94 and 96 over all 256 flag bytes × 6 latch values (clear
regardless of the latch when the own bit is set; bit isolation between the
two); composition latch → 93 clears → 93 inert → 94 still clears.

Measured: `cc -std=c11 -Wall -Wextra -Wpedantic -Werror` (gcc) builds the
library and test with no diagnostics and prints `overlay unit 0x38 helper
tests passed`; CMake build has no warnings; `ctest` passes 18 of 18 targets
on main 004e97b (seventeen existing plus `poolrad_overlay_unit38_helper_tests`).

## Limitations, stated plainly

- The game role of the helper and of `[0x6817]`/`[0x6818]` is not
  established; the five key values are compared immediates only. The
  routine's behavior is complete regardless.
- The other nine callers of the helper (entries 24, 50, 76, 91, 92, 95, 108,
  109 and the private `+0x2915`) are not translated here; their keys are
  listed above as evidence of the ABI only.
- Whether a caller's AH is ever nonzero is irrelevant to the original (byte
  compare) and to the translation (byte parameter), but the stack word is
  not modelled.
