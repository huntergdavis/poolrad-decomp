# Unit 0x1C +0x0994: provisional full-state INPUT STRING

ECL opcode `0x10` dispatches here ([ECL_CONTRACTS.md](ECL_CONTRACTS.md)).
The ignored DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The exact handler is file `[0x16DA,0x174F)`, unit `+0x0994..+0x0A08`,
**117 bytes / 49 instructions**, SHA-256
`cf8bd60da6a03799c2143ea2d71eb99dcf3a9ad994b31c2a5db01a771fb42ac9`.
It ends with `RETF` at `+0x0A08`; the next prologue starts at `+0x0A09`.
The preceding two bytes at `+0x0992` are `01 20`, the one-character Pascal
space string used by the empty-input branch. A bounded pre-implementation
`deja` query timed out without a result. Repository leads reused and checked
against the original bytes were [FUNCTION_LEDGER.md](FUNCTION_LEDGER.md)
for the unit-25 store-string ABI, [U1C_INPUT_NUMBER_FULLSTATE_0951.md](U1C_INPUT_NUMBER_FULLSTATE_0951.md)
for the loader/word call pattern, and [resident_inventory.py](../../tools/overlay/resident_inventory.py)
for the resident file mapping.

The handler reserves `0x204` local bytes and loads two operands. It assembles
the destination address from `DS:[7087]` and `DS:[7047]`, retaining AX's high
byte in each pushed word. It calls resident `0709:09E3` with an output Pascal
string at `SS:BP-0x204`, a choice byte at `SS:BP-2`, a low-byte limit `10`,
and maximum length `40`. That routine's `RETF 8` deliberately retains the
first far output pointer on the stack. The handler then copies this source to
`SS:BP-0x104` with maximum `255`. If that copied string has length zero, it
copies the `CS:0992` one-space string to the same local. Finally it passes
the local far pointer and destination address to unit-25 store string.

| Call site | Target | Confirmed cleanup | Open effects |
| --- | --- | ---: | --- |
| `+099E` | `2B:002A` loader | `RETF 2` | loader state and globals |
| `+09AF` | `2B:004D` operand word | `RETF 4` | operand value and flags |
| `+09C8` | `0709:09E3` input | `RETF 8` | interactive string, choice, resident effects |
| `+09D7`, optional `+09F2` | `AF8:0B25` copy | `RETF 10` each | runtime register/flag and string effects |
| `+0A00` | `2B:005C` store string | `RETF 6` | destination window and downstream hooks |

The resident input target maps to expanded START.EXE image offset `0x78AC`
(file `0x7AAC`) under the inventory's `-455` shift. Its first instruction is
`PUSH BP`; the first linear `RETF 8` at `+0x0B63` ends file span
`[0x7AAC,0x7C2F)`, 387 bytes / 165 instructions, SHA-256
`822b4aeac02e3396c177fbdb18187c6040862ec3595d7ffde16b8eb221565baf`.
SYSTEM copy is file `[0xB79B,0xB7BF)`, 36 bytes / 16 instructions,
SHA-256 `009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118`;
its final instruction is `RETF 0xA`. The unit-25 store-string
`2B:005C` still has the unresolved runtime/window dependencies described in
[FUNCTION_LEDGER.md](FUNCTION_LEDGER.md), so the full-state model exposes all
callee effects as explicit fail-closed callbacks.

`check_overlay_u1c_input_string_0994.py` pins the original overlay hash,
handler hash, 49 decoded instructions, six call sites, literal bytes and next
prologue. It compares original-byte Unicorn execution with the C wrapper in
18 cases: empty, one-character and 40-character input; separated and aliased
DS/SS; stack offset wrap; and store mutation of saved BP/far return. Every
case compares the full 1 MiB RAM, all 14 CPU fields and every call-entry
CPU/argument trace. The focused strict C test exercises both branches,
argument/return behavior and abort at every reached callback boundary.

No reached native `+0x0994` entry/return pair was found in a bounded search
of local text captures. Actual resident input, SYSTEM copy and store-string
live effects, as well as C-native parity, remain open. This is **provisional**;
it requests no strict full-C, new identity, or count credit.
