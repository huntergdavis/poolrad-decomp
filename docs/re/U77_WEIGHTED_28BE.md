# Unit 0x77 entry 11, +0x28BE: provisional weighted dword helper

`src/recovered/overlay_u77_weighted_28be.c` models the returned `DX:AX`
value of this routine. It is **provisional**: the two external Turbo Pascal
arithmetic calls are supplied as 32-bit multiply and divide in the
original-byte parity audit, and no native frame has been reached.

## Source and extent

The ignored DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Entry 11 of unit `0x77` is at file offset 145,656 (`+0x28BE`, stub
`0x85:0x57`). The ledger's 222-byte bound `[145656,145878)` has SHA-256
`aa2a58ca02e77cb9ceebd2247f3291fcb7e531832a5856af9b5322ec06218934`.
Capstone decodes the reachable code from `+0x28BE` through `retf 4` at
`+0x2936..+0x2938`: **123 bytes**. The remaining 99 bytes, `+0x2939` through
`+0x299B`, are length-prefixed text data used by the next entry `+0x299C`.
The code SHA-256 is `39dbab268eac0658980e3ef072352603eefac1cfb2b532412376cf2e970081c4`.
The two direct branches stay inside the code; the calls at `+0x28FD` and
`+0x2922` target runtime `AF8:A5B` and `AF8:A76`.

The routine receives a far record pointer at `[BP+6]`. It reads five words
from that record at `+0x88,+0x8A,+0x8C,+0x8E,+0x90`, zero-extends each to a
dword, multiplies each by a dword at `DS:0D24+4*i`, accumulates modulo
`2^32`, adds 100, and divides by the dword at `DS:0D30`. It returns the
quotient in `DX:AX` and performs no record or DS writes. A local captured
RAM image has the five weights `1,10,100,200,1000`; the fourth weight is
also the divisor. The model accepts all five weights as inputs because
the code reads them from RAM.

The caller relationship and record context were reused from
[U7F_DIV5_ENTRY10.md](U7F_DIV5_ENTRY10.md) and independently checked
against the entry bytes. A bounded `deja` query for this entry timed out
without a result; no prior-session result was reused.

## Checks and limits

`test_overlay_u77_weighted_28be.c` pins zero, one, maximum-word, fifth-slot,
short-buffer, null, and zero-divisor behavior. The original-byte audit
`check_overlay_u77_weighted_28be.py` verifies the source hash, code/data
boundary and branch targets; executes the original 16-bit instructions with
explicit 32-bit multiply/divide stand-ins at the two runtime calls; and
compares 40 returned dwords against the C model, including high word values.
It does not assert parity for scratch-stack bytes, runtime flags, or divide
errors. The input words and canonical weights keep the numerator positive
and below signed-32 overflow, so signed and unsigned division agree there.

A bounded search of native notes and 17 local RAM captures found no entry
frame or loaded entry-byte hit. This is therefore **not a native reached
frame**, a strict full-function parity claim, or a count claim. The next
entry `+0x299C` has its own separate implementation and evidence lane.
