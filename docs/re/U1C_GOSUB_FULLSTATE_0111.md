# Unit 0x1C +0x0111: provisional full-state GOSUB wrapper

The verified-game ECL opcode `0x02` dispatches here. Ignored DOS v1.3
`GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The exact body is `[0x0E57,0x0E6C)`, unit offsets `+0x0111..+0x0125`,
**21 bytes / 10 instructions**, SHA-256
`7917b6be7b4ae2f3d53de4c2a9d80ec430fba207ca34e689e88ac2186c4b0edc`.
The final `retf` ends at `+0x0126`, the next handler prologue. A bounded
`deja` query timed out without a result. The existing path-complete semantic
array model and its uncounted dependency are documented in
[FUNCTION_LEDGER.md](FUNCTION_LEDGER.md) section 1.8; the ECL dispatcher
identity is in [ECL_CONTRACTS.md](ECL_CONTRACTS.md).

The original wrapper saves BP, calls `2B:002A` loader with slot count 1,
then calls `2B:0075` push-call-frame with slot 1. Both are far Pascal calls
with `retf 2`. The new separate C model preserves the complete 1 MiB RAM,
CPU registers and flags, both argument pushes, BP restoration, far return,
and stack/DS alias effects. Explicit fail-closed callbacks supply the
loader and heap-backed push-call-frame effects. Those callees are not
silently substituted with the bounded host array model.

`check_overlay_u1c_gosub_0111.py` pins the original bytes, instruction and
call boundaries, then compares original-byte Unicorn execution with C in
12 cases. It compares complete RAM, returned CPU, and both call-entry CPU
and argument traces. Cases include AX high-byte carry-through, DS:SS alias,
stack offset wrap, PC/list-head callback changes, and callee modification
of saved BP and the far return words. The strict C test checks call order,
argument ABI, frame effects, and failure at each callback boundary.

A bounded local search found no reached native `+0x0111` entry frame; the
documented GOSUB route in the host ECL model is not such a frame. Actual
loader/heap effects and native caller parity remain open. This model is
**provisional** and requests no strict full-C, new identity, or count credit.
