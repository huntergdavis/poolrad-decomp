# Unit 0x1C +0x0AED: provisional full-state COMPARE AND

The verified-game ECL opcode `0x14` dispatches to this handler. The ignored
DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The exact body is `[0x1833,0x1898)`, unit offsets `+0x0AED..+0x0B51`,
**101 bytes / 40 instructions**, SHA-256
`c45465b78fc299d0f7e52d7bab18c58568e7c5c87759ae9f231561c35c24161f`.
The final `retf` ends at `+0x0B52`, the next handler prologue. The bounded
`deja` query timed out without a result; no prior-session result was reused.
The dispatcher identity and ECL operand conventions come from
[ECL_CONTRACTS.md](ECL_CONTRACTS.md); the slot-value boundary follows
[U1C_AND_OR_0DEF.md](U1C_AND_OR_0DEF.md).

The body calls runtime `AF8:16A8` FillChar with `DS:6F78`, six bytes, zero;
calls `2B:002A` to load four operands; calls `2B:0025` four times for their
word values; and compares values 1/2 then 3/4. It sets `[6F78]` when both
pairs match and `[6F79]` otherwise. The standalone C model preserves the
10-byte stack frame, byte loop counter, four word locals, CPU arithmetic
flags, far return, and all RAM writes, including SS:DS aliasing. Explicit
callbacks supply the complete external callee effects; a rejected callback
stops at its exact call boundary. The known argument cleanups are RETF 8 for
FillChar and RETF 2 for loader and slot value.

`check_overlay_u1c_compare_and_0aed.py` checks the source hash, decoded
boundary, and all call sites, then compares original bytes under Unicorn to
the C model across 20 cases. It checks the complete 1 MiB RAM, returned CPU,
and call-entry CPU/argument traces; AF after XOR/SHL is masked because x86
leaves it undefined. Cases include both unequal exits, equal pairs, signed
word boundaries, callback RAM/CPU changes, and SS:DS aliasing.
`test_overlay_u1c_compare_and_0aed.c` checks both exits and the callback ABI
under the strict C build. A bounded local search found no reached native
`+0x0AED` entry frame. Actual loader/slot-value live effects and native caller
parity remain open. This is **provisional**: no strict full-C, identity, or
count credit is requested.
