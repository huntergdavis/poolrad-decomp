# Unit 0x1C +0x2AC1: provisional ECL CLOCK handler

This is the verified-game ECL opcode `0x34` dispatch start. The C model covers
its complete decoded body and exposes three far calls as callbacks. The
operand loader and slot-value calls have existing path-complete models; unit
`0x7D` entry 1 remains bounded. No native entry frame or joint callee replay
was reached, so this is **provisional** and requests no strict full-C credit,
new verified-identity claim, or count claim.

## Source, caller, and dependencies

Ignored DOS v1.3 `GAME.OVR`: 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x1C` begins at file `0x0D46`. This routine is the exact 42-byte range
`[0x3807,0x3831)`, unit `+0x2AC1..+0x2AEA`, SHA-256
`94a44f80f35c4b673eee1a45fe0869e383400087e6659a952b223c3df6d43420`:
18 instructions through the final-byte `retf`. The following routine begins
at `+0x2AEB`. The ECL dispatcher maps opcode `0x34` to this near-call start;
[ECL_CONTRACTS.md](ECL_CONTRACTS.md) records it.

The calls are `2B:002A` operand loader at `+0x2ACA` (`retf 2`), `2B:0025`
slot value at `+0x2AD3` (`retf 2`), and `8C:0025` unit `0x7D` entry 1 at
`+0x2AE2` (`retf 4`). Existing contracts are in
[FUNCTION_LEDGER.md](FUNCTION_LEDGER.md) and [U7D_ENTRY1.md](U7D_ENTRY1.md).
The handler pushes count 1 to the loader; reads its uninitialized local byte
`[BP-1]` as the slot index; stores the value callee's returned AL there; then
calls unit `0x7D` with `(n=that byte, idx=1)`. Its pushes preserve the current
AH, so both argument words are kept full-width in the model. The `sub sp,2`
prologue sets arithmetic flags before the first call; the model preserves
those and all later callee-supplied flags.

Unit `0x7D` entry 1 rewrites window-0 words `+0x018C..+0x0199`, including
word `+0x018E` for index 1, under its own bounded dependency. This CLOCK
route is distinct from U1C `+0x271F` DELAY, which calls resident `0709:0DBE`
and then `A40:029C`; p9 and PM confirmed no reached native `+0x2AC1` frame.
Two bounded `deja` queries timed out, so no prior-session result was reused.

## Verification and open gate

`check_overlay_u1c_clock_2ac1.py` hashes the original file/body, checks all
18 instruction boundaries and call/return sites, then compares 16 Unicorn
original-byte runs with the C model from identical states. It checks the
**entire 1 MiB RAM**, all returned CPU registers/flags, and callback entry
registers plus stack arguments. Cases vary initial local byte, AH, slot-value
return, and loader modification of the local and registers. The same bounded
call effects are supplied to both executions. The focused C test checks the
three full-width call arguments, callback order, local write, flag state at
the first call, and far return.

Native caller/callee parity and unit `0x7D` entry-1 dependency parity remain
open. The PR makes no strict or count claim.
