# Unit 0x1C +0x00F4: provisional full-state ECL GOTO

The verified-game ECL opcode `0x01` dispatches here. Ignored DOS v1.3
`GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The exact body is `[0x0E3A,0x0E57)`, unit offsets `+0x00F4..+0x0110`,
**29 bytes / 13 instructions**, SHA-256
`da02d8926de5945de103e7c90ee1ba9856c645e82f531193ba40eb0f28a2779b`.
The final `retf` ends at `+0x0111`, the GOSUB prologue. A bounded `deja`
query timed out without a result. The existing path-complete semantic model
and its uncounted loader dependency are documented in
[FUNCTION_LEDGER.md](FUNCTION_LEDGER.md) section 1.8; dispatcher identity is
in [ECL_CONTRACTS.md](ECL_CONTRACTS.md).

The handler calls `2B:002A` operand loader with count 1 (RETF 2), reads
`DS:[7086]` and `DS:[7046]` into AL in turn, pushes the **full AX** each
time, calls `2B:004D` operand-word assembly (RETF 4), and writes its AX result
to script PC `DS:[49ED]`. The word helper is separately strict in ledger
section 1.1; the loader remains a dependency. This separate C model preserves
the complete RAM, CPU and flags, argument-stack words, SS:DS alias effects,
callee-modified return frame, and far return. External effects are explicit
fail-closed callbacks. It does not duplicate the existing host GOTO identity.

`check_overlay_u1c_goto_00f4.py` pins the source/body hashes and decoded
boundaries, then compares original-byte Unicorn execution with C in 18 cases:
complete 1 MiB RAM, returned CPU, and both call-entry CPU/argument traces.
Cases include byte extremes, AX high-byte carry-through, DS:SS alias, stack
offset wrap, slot mutation after argument capture, and helper modification of
saved BP and far return words. The strict C test checks call order, Pascal
arguments, script-PC store, and both callback abort boundaries.

A bounded local search found no reached native `+0x00F4` entry frame; ECL
host-route execution is not one. Native caller and actual loader live effects
remain open. This model is **provisional** and requests no strict full-C, new
identity, or count credit.
