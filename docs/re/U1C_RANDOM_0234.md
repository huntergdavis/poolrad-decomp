# Unit 0x1c +0x0234: ECL RANDOM (0x08), bounded model

Opcode `0x08` ([ECL_CONTRACTS.md](ECL_CONTRACTS.md), 2 operands) dispatches to unit `0x1c`
`+0x0234..+0x0291`, file `0x0F7A..0x0FD7`, 94 bytes, 35 instructions, four bytes of locals,
`retf`. SHA-256 of the span
`a634c69e97fe005c2041e9d18d0874d0eb8ede7f4a59833bfb642918b7c5d979` (GAME.OVR, 206,105 bytes,
`872964a3…7928614`). The next routine (`+0x0292`) is SAVE. **Classification: bounded model**;
the three runtime calls are opaque, no generator semantics and no saved result are modelled.
No count, strict full-C, identity or DOS/native parity is claimed. `deja` found nothing earlier.

## Original order (all 35 instructions checked against the bytes)

| Range | Effect |
| --- | --- |
| `+0x023A..+0x023D` | loader(2), `0x2B:0x2A` |
| `+0x0242..+0x024A` | `v := AL` of slot value(1), `0x2B:0x25` |
| `+0x024D..+0x0253` | `cmp byte v, 0xFF; jae +0x0256`, else `inc byte v` (saturating at 0xFF) |
| `+0x0256..+0x0263` | `dest := word([0x7087], [0x7047])` via `0x2B:0x4D` (slot 2) |
| `+0x0266..+0x026C` | push the word `v` (`xor ah, ah`); `lcall 0xAF8:0x11C0` |
| `+0x0271..+0x0273` | `xor dx, dx`; `lcall 0xAF8:0x119A` |
| `+0x0278..+0x027D` | `lcall 0xAF8:0x119E`; `[bp-2] := AL` |
| `+0x0280..+0x0289` | push `dest`, push the zero-extended AL; `lcall 0x2B:0x52` (store word) |
| `+0x028E..+0x0291` | return |

## Model

`pr_u1c_op_random` (`overlay_u1c_random_0234.{h,c}`): the three `0xAF8` calls are separate
fallible hooks in this order (NULL = HOOK, nonzero = CALLBACK, neither stores anything).
The hook ABI is only what the instructions show: the first gets the stack word, the
second the DX value (0), the third supplies AL; the registers one call leaves for the next
(AX/DX) are not decoded and not modelled. The store word reports its status after the
attempt (HOOK = word written). Loader and slot-value BOUNDS/UNRESOLVED stop before any
hook; a class-1 slot read runs to completion then reports HOOK (PICTURE pattern).

Tests (`test_overlay_u1c_random_0234.c`) are fully synthetic: every slot byte 0..255 for the
saturating increment, hook order and arguments, the zero-extended store; a failing and a
missing hook at each step with nothing stored; destination classes (window 2, window 0,
exempt and non-exempt class 1, unsupported class); loader and slot failures; a class-1
slot read; a word slot value (low byte). The fabricated hook result is not a generator.

## Private supplied-state probe (documented, not committed)

Hash-pinned slot A (`abca2483…c08f1`), run entry `0x9914`, search entry `0x9975`; handlers
bound: SAVE, COMPARE, IF, GOTO, AND/OR, EXIT, PICTURE, PRINT/PRINTCLEAR, ON GOTO/GOSUB,
PARTYSTRENGTH (explicit empty host list) and this one; explicit `[0x84E2] = 0`, PICTURE DS
bytes 0. **The RANDOM result is an explicit host input (AL = 0), not a saved or generated
value.** At `0x9B3A`: slot byte 13, argument 14, three hook calls, AL (host) stored as a
word to the window-2 word `0x980F` (OK).

**Next stop**: dispatch 25, run status HANDLER / handler HOOK, PC `0x9B46`, opcode `0x03`
COMPARE (previous opcode `0x08`), whose class-1 operand is a word load from `0x6DCA` (load
hook, not exempt; not audited). Supplied-state probe: no DOS reachability, display or
one-Up parity.
