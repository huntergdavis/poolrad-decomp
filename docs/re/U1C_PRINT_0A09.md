# Unit 0x1c +0x0A09: ECL PRINT (0x11) and PRINTCLEAR (0x12), bounded model

Opcodes `0x11` and `0x12` share one handler ([ECL_CONTRACTS.md](ECL_CONTRACTS.md)):
unit `0x1c` `+0x0A09..+0x0A9E`, file `0x174F..0x17E4`, 150 bytes, a 256-byte stack
local, `retf`. SHA-256 of the span
`e14b2134252480a0c064c7e6bdf7f9623935e41c0f3a3a23f95aad6fd16ea838` (GAME.OVR,
206,105 bytes, `872964a3…7928614`). The next routine (`+0x0A9F`) is RETURN.
**Classification: bounded model** with three unresolved far callees as fallible
host hooks. No count, strict full-C or DOS/one-Up parity is claimed.
`deja` found only ECL_CONTRACTS/ECL_NEW_GAME_ROUTE mentions; reused the
PICTURE/EXIT hook pattern (#179, #184).

## Original order

| Range | Effect |
| --- | --- |
| `+0x0A10..+0x0A13` | loader(1), `0x2B:0x2A` |
| `+0x0A18`, `+0x0A1D` | `[0x84E4] := 0`, `[0x4A00] := 1` |
| `+0x0A22` | `cmp [0x7006], 0x80; jae +0x0A4B` (tag of slot 1) |
| `+0x0A29..+0x0A38` (tag < 0x80) | slot value(1) `0x2B:0x25`; push far `ss:[bp-0x100]` and the value; `0xBA:0x1073` |
| `+0x0A3D..+0x0A46` | push far `DS:0x70C8` (string buffer 1) and `0xFF`; `0xAF8:0x0B25` |
| `+0x0A4B` | `cmp [0x6F7F], 0x11` (the latch) |
| `+0x0A52..+0x0A6E` (latch 0x11) | `0x709:0x0768` with byte words `1, 0x11, 0x26, 0x16, 0x0A, 0` and far `DS:0x70C8` |
| `+0x0A70..+0x0A91` (otherwise) | `[0x5E25] := 0x11`, `[0x5E24] := 1`, then the same call with sixth word `1` |
| `+0x0A96..+0x0A9E` | `[0x4A00] := 0`, return |

Stop effects: any failure keeps everything already done, and `[0x4A00]` is cleared
only after the print call returns.

## Model

`pr_u1c_op_print` (`overlay_u1c_print_0a09.{h,c}`): `BA:1073`, `AF8:0B25` and
`709:0768` are separate fallible hooks (NULL = HOOK, nonzero = CALLBACK, each
keeping prior effects). `AF8:0B25` is a reached runtime callback with the loader
state, destination buffer 1 and maxlen `0xFF`; the model does not copy for it and
no host-side copy is claimed (its callee is not decoded). The print hook receives
buffer 1 as the loader state's own `string_len[1]`/`string_text[1]` (no copy).
`[0x84E4]` and `[0x4A00]` are explicit host bytes. Core loader/slot BOUNDS and
UNRESOLVED stop before any hook (loader state and the two DS writes retained as
the original order dictates); a class-1 slot read runs to completion, then reports
HOOK. Argument widths are `mov al, imm; push ax`: upper bytes unstated and not
claimed.

Tests (`test_overlay_u1c_print_0a09.c`) are fully synthetic: both opcodes, string
and numeric tags, call order and arguments, a failing and a missing hook at each
step, core statuses, class-1 HOOK completion, no `[0x4A00]` clear on a stop.

## Private smoke on the restored slot-A state (documented, not committed)

Hash-pinned slot A (`abca2483…c08f1`), run entry `0x9914`, then the search entry
`0x9975` with the merged SAVE/COMPARE/IF/GOTO/AND-OR/EXIT/PICTURE and this handler
bound; explicit inputs `[0x84E2]=0`, PICTURE DS bytes 0, a counting print hook.

* Search entry: SAVE to `0x6DE1`, PICTURE (clear path), SAVE to `0x4A1F`, **PRINTCLEAR
  at `0x9984`** (operand tag `0x80`, length 0: the string path, no numeric hooks),
  AND/OR (`0x2F`) to `0x6E82`; all OK.
* PRINTCLEAR effects: `[0x84E4]=0`, `[0x4A00]=0`, `[0x5E25]=0x11`, `[0x5E24]=1`; one
  `709:0768` call, args `(1, 0x11, 0x26, 0x16, 0x0A, 1)`, string length 0.
* **Next stop**: dispatch 6, run status HANDLER / handler HOOK, PC `0x9996`, opcode
  `0x09` SAVE, previous opcode `0x2F`: a class-1 store to `0x6E79` (value written, the
  store hook not exempt). Not audited here.

This is a supplied-state probe: no DOS reachability, display, or one-Up parity.
