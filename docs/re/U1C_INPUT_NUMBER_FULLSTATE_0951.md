# Unit 0x1C +0x0951: provisional full-state INPUT NUMBER

The verified-game ECL opcode `0x0F` dispatches here
([ECL_CONTRACTS.md](ECL_CONTRACTS.md)). Ignored DOS v1.3 `GAME.OVR` is
206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The exact handler is `[0x1697,0x16D8)`, unit `+0x0951..+0x0991`,
**65 bytes / 26 instructions**, SHA-256
`6b903fd5ca5b3680cdf3624ed8cf74916b6f4bcf861af46c7e3198a94704d79d`.
The `retf` ends at `+0x0992`; two data bytes precede the next prologue at
`+0x0994`. A bounded `deja` query timed out without a result.

The body reserves six local bytes, calls `2B:002A` loader with count 2,
zeros `SS:[BP-2]`, assembles operand slot 2 from `DS:[7087/7047]` through
`2B:004D`, calls resident `0709:0B66` with the far pointer `SS:BP-2` and
low-byte limit 10, then passes the destination and returned number to
`2B:0052` store word. Its own in-body writes are the local byte and two
local words; all four callee effects remain explicit fail-closed callbacks.
The model preserves full RAM/CPU/flags, stack arguments and aliasing, and
far return. The resident target maps through
`tools/overlay/resident_inventory.py` to expanded START.EXE image offset
`0x7A2F` (file `0x7C2F`): the entry is a Pascal prologue and its final
`+0x0BF2` instruction is `retf 6`. Its 143-byte span through `+0x0BF4`
has SHA-256 `07cb6386ab7eca71adcedf7cf67431f269669beee026073a80c754198261f897`.
The loader uses RETF 2; operand word
and store word use RETF 4, as in
[FUNCTION_LEDGER.md](FUNCTION_LEDGER.md). The newer bounded
[U25_STORE_WORD_0C77.md](U25_STORE_WORD_0C77.md) closes local class-4 and
class-1 effects, but stops at three unit-3BD far calls; this handler keeps
the entire store callee as a callback.

`check_overlay_u1c_input_number_0951.py` pins source/body hashes and
instruction/call boundaries, then compares original-byte Unicorn execution
with C in 18 cases. It checks the complete 1 MiB RAM, returned CPU, and
all four call-entry CPU/argument traces, including AX high-byte carry-through,
SS:DS alias, stack offset wrap, input mutation of the local byte, and store
mutation of saved BP/far return. The strict C test checks call order,
arguments, local/PC effects, and abort at every call boundary.

No reached native `+0x0951` entry frame was found in a bounded local search.
Actual resident input and store-word live effects, plus native caller parity,
remain open. This model is **provisional** and requests no strict full-C,
new identity, or count credit.
