# Unit 0x1C +0x2B14: provisional CLEAR BOX handler

This is the verified-game ECL opcode `0x3D` dispatch start. The C model covers
the whole decoded body but retains resident display routine `0709:0DD3` as a
callback. No reached native entry frame or joint caller/callee replay is
available, so this is **provisional** and requests no strict full-C credit,
new verified-identity claim, or count claim.

## Provenance and call boundary

Ignored DOS v1.3 `GAME.OVR`: 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x1C` begins at file `0x0D46`. The handler is the exact 31-byte range
`[0x385A,0x3879)`, unit `+0x2B14..+0x2B32`, SHA-256
`69641d023ee5478d48a8dd54832eda981a2d9cb1c87aeff0d08435e27ae3c0bf`:
15 instructions through the last-byte `retf`, followed by a new prologue at
`+0x2B33`. The ECL dispatcher maps opcode `0x3D` to this near-call start;
[ECL_CONTRACTS.md](ECL_CONTRACTS.md) records that mapping. The sole outgoing
call is resident `0709:0DD3`, also used by
[U1C_HORIZONTAL_MENU_10D1.md](U1C_HORIZONTAL_MENU_10D1.md).

The ignored `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Its mapped resident target has a Pascal frame at `0DD3` (file `0x7E9C`) and
an epilogue `mov sp,bp; pop bp; retf 8` at `0F7C..0F7F`. This independently
confirms cleanup of the four pushed argument words; its display effects are
still callback-bounded. Two bounded `deja` queries for the assigned starts
timed out, so no prior-session result was reused.

## Decoded body and verification

The handler increments ECL PC word `DS:[49ED]`, pushes four words with low
bytes `1,1,15,15` in that order, calls the resident display routine, writes
`1` to `DS:[84DE]`, and returns. The four pushes preserve the incoming AH;
the callback sees the words in reverse order at its stack top. The PC `INC`
updates OF/SF/ZF/AF/PF and preserves CF.

`check_overlay_u1c_clear_box_2b14.py` hashes the original file/body, checks
all 15 instruction boundaries and the call/return, then compares 16
original-byte Unicorn runs with C from identical initial states. It checks
the **entire 1 MiB RAM**, all returned CPU registers and flags, and callback
entry registers plus all four stack arguments. Cases cover PC zero, low-nibble
carry, signed overflow, and wrap; two AH values; two initial flag patterns;
and a callback mutation. The same bounded display effects are supplied to
both executions. The focused C test checks API guards, all four full-width
arguments, overflow flags, the latch write, and far return.

No native `+0x2B14` entry frame was found in the bounded local route notes and
captures. The resident display dependency is not jointly translated here.
