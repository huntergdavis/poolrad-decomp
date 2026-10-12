# Unit 0x1c +0x12C6: ECL PARTYSTRENGTH (0x1D), bounded model

Opcode `0x1D` ([ECL_CONTRACTS.md](ECL_CONTRACTS.md), 1 operand) dispatches to unit `0x1c`
`+0x12C6..+0x13D6`, file `0x200C..0x211C`, 273 bytes, twelve bytes of locals, `retf`.
SHA-256 of the span `d05d638ad745ffead625d9983e1ad6751d0ce20254c40cef299403b7fad18058`
(GAME.OVR, 206,105 bytes, `872964a3…7928614`). The next prologue (`+0x13D7`) is a separate
routine, excluded. **Classification: bounded model**; the reference name is not a game-role
claim. No count, strict full-C, identity or DOS/native parity is claimed. `deja` found
nothing earlier for this handler.

## Original order (all 100 instructions checked against the bytes)

| Range | Effect |
| --- | --- |
| `+0x12CC..+0x12CF` | loader(1), `0x2B:0x2A` |
| `+0x12D4` | `sum := 0` (byte `[bp-6]`) |
| `+0x12D8..+0x12E2` | `node := far [0x5D96]:[0x5D98]` (offset word, segment word) |
| `+0x12E5..+0x12ED` | `or` of the two words; zero -> exit to `+0x13B5` |
| `+0x12F0..+0x1324` | byte reads through `es:[di + k]`: `a := +0x11B`, `b := +0x111`, `c := +0x110`, `d := +0x09B`, `e := +0x096` |
| `+0x1327..+0x133A` | `b > 0x3C` (unsigned `jbe`): `b := b - 0x3C`, else `b := 0` |
| `+0x133E..+0x1351` | `c > 0x27`: `c := c - 0x27`, else `0` |
| `+0x1355..+0x1394` | `w := (d << 3) + 5c + 5b + a + (e << 2)` (16-bit register arithmetic) |
| `+0x1396..+0x139C` | `cwd` (opcode `0x99`, 16-bit; `DX := 0`), `div cx` (10); `sum := sum + AL` (byte add of the low byte of the quotient) |
| `+0x139F..+0x13B2` | `node := far pointer at +0x104 (offset) and +0x106 (segment)`; loop |
| `+0x13B5..+0x13C2` | `dest := word([0x7086], [0x7046])` via `0x2B:0x4D` |
| `+0x13C5..+0x13CE` | `0x2B:0x52 (dest, zero-extended sum)`, the store word call |

16-bit and divide behavior: `w` is at most `255 + 5*195 + 5*216 + 8*255 + 4*255 = 5,370`,
so no 16-bit wrap and the `cwd` sign extension makes `DX = 0`; the divisor is the constant 10 (no
fault). The quotient can exceed 255 (up to 537) and only its low byte is added to the
byte accumulator, which wraps modulo 256.

## Model and missing state

`pr_u1c_op_party_strength` (`overlay_u1c_party_strength_12c6.{h,c}`) composed of the
lane-A loader, `pr_rec_ecl_operand_word` and `pr_rec_ecl_store_word`. The **party list is
not in the model's state**: it is explicit host input, using the same host node type as the
unit `0x68` party-byte routine (`pr_u68_0236_node`: host bytes of the node's segment, the far
offset, the next link). Field reads are `view[(offset + k) mod 65536]`; a null or too small
view or a list past the cap (the original has no cycle check) is a host guard (status LIST)
with nothing stored. Failure order: loader failure first (the list is not touched);
list failure after the loader, before any store; the store word's HOOK/BOUNDS/UNRESOLVED is
reported after the store attempt (a HOOK means the window word was written).

Tests (`test_overlay_u1c_party_strength_12c6.c`) are fully synthetic: the full product of
eleven boundary values for the five fields of one node and 3,000 pseudo-random lists,
against an independent integer restatement; byte wrap with forty maximal nodes; nil head; the
16-bit offset wrap in a 64 KiB view; null, too-small and cyclic host views; window-2,
window-0 and class-1 (exempt and non-exempt) destinations and an unsupported class;
loader failure with a bad node never reached.

## Private supplied-state probe (documented, not committed)

Hash-pinned slot A (`abca2483…c08f1`), run entry `0x9914`, then search entry `0x9975` on the
retained state; handlers bound: SAVE, COMPARE, IF, GOTO, AND/OR, EXIT, PICTURE,
PRINT/PRINTCLEAR, ON GOTO/GOSUB and this one; explicit `[0x84E2] = 0`, PICTURE DS bytes 0.
**The party list is an explicit host input, not saved data:** with the empty list the
handler visits 0 nodes and stores 0 to the operand word `0x9808` (OK); with a one-node
fabricated list (field `+0x11B` = 100) it stores 10. Nothing from the save's character
records is used.

Search entry from the ON GOTO target `0x9AA5` (dispatch 14), with the empty list:
PARTYSTRENGTH, COMPARE, IF, COMPARE, IF (taken to `0x9B24`), SAVE, COMPARE, IF, COMPARE, IF
(`0x1B` at `0x9B38`, not taken, the skipped one-byte instruction being an EXIT).
**Next stop**: dispatch 24, run status UNMODELLED, PC `0x9B3A`, opcode `0x08` (RANDOM,
handler `+0x0234`, 2 operands). The path after PARTYSTRENGTH depends on the supplied list:
this is a supplied-state probe with no DOS reachability, display or one-Up parity.
