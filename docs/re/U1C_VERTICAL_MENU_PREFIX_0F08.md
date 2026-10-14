# Unit 0x1C +0x0F08: bounded full-state VERTICAL MENU prefix

ECL opcode `0x15` dispatches here ([ECL_CONTRACTS.md](ECL_CONTRACTS.md)).
The ignored DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The complete handler's instruction body is file `[0x1C4E,0x1DCB)`, unit
`+0x0F08..+0x1084`, **381 bytes / 149 instructions**, SHA-256
`937fbfd3ae61653216beaff6084175a35be1b22c3f0c4158d5379463c7d12288`.
The `RETF` ends at `+0x1085`, followed by embedded text before the next
`+0x10D1` HORIZONTAL MENU prologue. A bounded pre-implementation `deja`
query timed out without a result. Reused repository leads, checked against
the original bytes, were [U1C_HORIZONTAL_MENU_10D1.md](U1C_HORIZONTAL_MENU_10D1.md)
for adjacent menu behavior, [FUNCTION_LEDGER.md](FUNCTION_LEDGER.md) for
the unit-25 operand calls, and [SLOT_A_STRING_0425_ABI.md](SLOT_A_STRING_0425_ABI.md)
for SYSTEM copy's `RETF 10` ABI.

This model covers exactly file `[0x1C4E,0x1CB9)`, unit
`[+0x0F08,+0x0F73)`: **107 bytes / 39 instructions**, SHA-256
`3d8da2c70bec5698c5cf0349d5355fd0d6dba50c008fa51e01e8aafc2a5623e5`.
It stops with `CS:IP` at `+0x0F73`, **before** the original five-byte
`LCALL 3C5:003E` (`9A 3E 00 C5 03`) executes. It preserves the full 1 MiB
RAM, 14 CPU fields, all pushed arguments and the exact callback entry state.
The stop is a dependency boundary: that callee receives a count and an
`SS:BP-0x106` output pointer, and later body instructions read two words
from the output. Its complete effects and reached native result are not
established here.

Before that stop, the prefix reserves `0x112` local bytes, clears
`DS:[84E4]`, seeds three local control bytes, loads three operands, reads
one word from the first operand slot, copies the Pascal string at
`DS:70C8` into `SS:BP-0x102` with maximum 255, gets a byte count from
the second operand slot, decrements the script PC word `DS:[49ED]`, reloads
that many operands, and pushes the count and far output pointer for the
pending call. The word decrement preserves CF; `XOR AH,AH` clears the
arithmetic flags and sets ZF/PF before the stop.

| Site | Target | Cleanup after callback |
| --- | --- | ---: |
| `+0F25` | `2B:002A` loader(3) | `RETF 2` |
| `+0F32` | `2B:004D` operand word | `RETF 4` |
| `+0F4A` | `AF8:0B25` Pascal copy | `RETF 10` |
| `+0F52` | `2B:0025` slot value | `RETF 2` |
| `+0F62` | `2B:002A` loader(count) | `RETF 2` |

All five callee effects are explicit fail-closed callbacks. The original-byte
Unicorn checker compares C against all 107 original bytes in 18 cases:
empty/short/40-character source strings, script PC values around zero and
signed overflow, DS/SS aliasing, stack offset wrap, and a second-loader
mutation of the local count before the final push. It checks complete 1 MiB
RAM, all 14 CPU fields and five call-entry CPU/argument traces. The focused
strict C test checks callback order and arguments, local/stack stop state,
and abort at every call boundary.

The remaining handler contains a linked-list traversal, additional menu and
display calls, and the final store. No reached native `+0x0F08` frame was
available in a bounded local capture search. This prefix is **provisional**:
no complete handler, strict full-C, new identity, or count credit is claimed.
