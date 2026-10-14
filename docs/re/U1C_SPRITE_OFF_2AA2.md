# Unit 0x1C +0x2AA2: provisional SPRITE OFF handler

This is the verified-game ECL opcode `0x31` dispatch start. The C model covers
the whole decoded body but retains its unit `0x3B4` view-refresh call as a
callback. No reached native entry frame or joint caller/callee replay is
available, so this is **provisional** and requests no strict full-C credit,
verified-identity uplift, or count claim.

## Provenance and route

Ignored DOS v1.3 `GAME.OVR`: 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x1C` begins at file `0x0D46`. This function is the exact 31-byte
range `[0x37E8,0x3807)`, unit `+0x2AA2..+0x2AC0`, SHA-256
`4b1715dc38485f2880c28b16c1e3e0a7887086d488173d70f9e70dd863f2b25c`:
10 instructions through the last-byte `retf`, followed by the next function
at `+0x2AC1`. The ECL dispatcher maps opcode `0x31` to this near-call start;
[ECL_CONTRACTS.md](ECL_CONTRACTS.md) and the overlay function ledger provide
that mapping. The only outgoing call is `03CA:0025`, the unit `0x3B4` entry-1
view refresh identified in [U1C_PICTURE_0856.md](U1C_PICTURE_0856.md) and
[U1C_LOOP_3387.md](U1C_LOOP_3387.md). Two bounded `deja` queries for the
assigned starts timed out, so no prior-session result was reused.

## Decoded body and verification

The handler increments the ECL PC word `DS:[49ED]`. If dirty byte
`DS:[84E3]` is zero it returns. Otherwise it calls view refresh, then clears
`DS:[84E3]` and `DS:[84E0]` before returning. The callback receives the
original CPU/RAM at the far-call site and can supply callee effects.

`check_overlay_u1c_sprite_off_2aa2.py` hashes the original file/body,
checks all 10 instruction boundaries and the call/branch/return, then compares
16 original-byte Unicorn runs with C from identical initial states. It checks
the **entire 1 MiB RAM**, all returned CPU registers and flags, and callback
entry registers. Cases include clean and dirty flags, positive and negative
dirty bytes, ECL PC wrap, two initial flag patterns, and a callback mutation.
The callback is bounded and identical in both executions. The focused C test
checks API guards, the dirty branch, PC wrap, byte clears, and far return.

No native `+0x2AA2` entry frame was found in the bounded local route notes and
captures. The view-refresh dependency is not jointly translated here.
