# Unit 0x1c +0x0E5E: ECL GETTABLE (0x2A), bounded model

Opcode `0x2A` (GETTABLE, reference name; [ECL_CONTRACTS.md](ECL_CONTRACTS.md), 3 operands)
dispatches to unit `0x1c` `+0x0E5E..+0x0EBB`, file `0x1BA4..0x1C01`, 94 bytes (the `retf` at
`+0x0EBB`), ten bytes of locals. The next routine (`+0x0EBC`, a loader(3) routine) is separate, as is
`+0x10D1` (HORIZONTAL MENU) further on. SHA-256 of the span
`30aabe87e9b1c4ac74b51cc202f2657be9ca70e40df0f76d55b2f0010c554db1` (GAME.OVR, 206,105 bytes,
`872964a3…7928614`). `deja` found no earlier note. **Classification: bounded model** composed of lane-A
pieces; no new hooks. No count, strict full-C, identity, DOS/native or one-Up parity claim.

## Original order (all instructions checked against the bytes)

| Range | Effect |
| --- | --- |
| `+0x0E64..+0x0E67` | loader(3), `0x2B:0x2A` |
| `+0x0E6C..+0x0E79` | `base := word([0x7086], [0x7046])` via `0x2B:0x4D` (slot 1's value bytes, whatever its tag) |
| `+0x0E7C..+0x0E84` | `idx := AL` of slot value(2) (`0x2B:0x25`): **the low byte only** |
| `+0x0E87..+0x0E94` | `dest := word([0x7088], [0x7048])` via `0x2B:0x4D` (slot 3) |
| `+0x0E97..+0x0E9F` | `addr := idx (zero-extended) + base` (`add ax`, 16-bit) |
| `+0x0EA2..+0x0EAA` | `value := 0x2B:0x57 (addr)` |
| `+0x0EAD..+0x0EB3` | `0x2B:0x52 (dest, value)`, the store word call; return |

`0x2B:0x57` is unit `0x25` entry 11 (stub offset `0x25 + 5*11`), the load word at `+0x0E0C` (the inventory
entry list gives file offset `3,596` from the unit base, `+0x0E0C`); `0x2B:0x52` is entry 10, the store word
(`+0x0C77`); `0x2B:0x4D` is entry 9, the operand word. The handler carries **no table**: the element is whatever the
lane-A load word returns at `addr` (window 0/1/2 word, the **zero-extended script byte** for the script buffer
class, a class-4 pseudo-address, or a class-1 address through the engine's load hook).

## Model

`pr_u1c_op_gettable` (`overlay_u1c_gettable_0e5e.{h,c}`): a loader failure stops first; a slot value(2)
BOUNDS/UNRESOLVED stops before the element load; a class-1 slot read completes then reports HOOK; the load
word's status is reported (a class-4 address the model cannot resolve stops before the store, where the engine
would store an uninitialized local; a class-1 element address reads and stores, then HOOK); the store word's
status is reported after the attempt.

Tests (`test_overlay_u1c_gettable_0e5e.c`) are fully synthetic: all 256 index bytes over a window-0 table,
window-2 and script-buffer tables (byte element), the low-byte-only index (a word slot value `0x01FF` indexes
`0xFF`), slot 1 as an address word with a literal tag and a stale high byte, the 16-bit address wrap, class-1
(exempt and not) and class-4 (resolved and unsupported) element addresses, destination classes, loader and slot
failures, and a class-1 slot 2 read.

## Private supplied-state probe (documented, not committed)

Hash-pinned slot A (`abca2483…c08f1`), run entry `0x9914`, search entry `0x9975`, the six-record host party of
[SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md), merged handlers bound, explicit host inputs none from DOS:
`[0x84E2] = 0`, PICTURE DS bytes 0, first RANDOM result 13, second RANDOM result AL2 in {0, 1, 2}.

GETTABLE at `0x9BCF`: slot 1 is the address word `0xB6E3` (inside the saved script buffer, so the table is the
save's own bytes, class 3), slot 2 is the class-1 word `0x6E7F` (the second RANDOM result, via the plain load
hook), `dest` is `0x6DC1`. For AL2 = 0, 1, 2 the addresses are `0xB6E3`, `0xB6E4`, `0xB6E5` and the loaded elements
are all `2` (the saved script bytes there are 2, 2, 2). The element is stored to the class-1 word `0x6DC1` (the word is written).

**Next stop** (all three): HANDLER / HOOK at PC `0x9BD9` after the GETTABLE (dispatch 39; 38 for AL2 = 2): the
store to `0x6DC1` is a class-1 store whose hook is not exempt. Not audited here (the private follower finds
`0x6DC1` quiet). Supplied-state probe only: no live DOS RNG, native search reachability or one-Up parity.
