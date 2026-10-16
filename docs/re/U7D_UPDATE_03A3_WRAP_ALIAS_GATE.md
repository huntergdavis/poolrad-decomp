# U7D +03A3 outer-return wrap with live aliases

This focused C gate exercises the already merged complete U7D `+03A3`
source and its merged complete `+02F2` callee. It adds no translation.
The [caller composition](U7D_UPDATE_03A3.md) and
[carry normalizer](U7D_CARRY_02F2.md) are the source and ABI evidence;
PM assigned this separate wrap/alias closure screen after PR #606
(i7701). The exact `deja` query for SP FFFE, DS:6DCC and saved-BP alias
was stopped after exceeding the bounded recall interval; the earlier
U7D closure recall found no indexed exact session.

Ignored `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The +02F2 callee is file `[0x240BE,0x2416F)`, 177 bytes / 69 starts,
SHA-256
`8f39276ad357b6e05367ffc080afedc0df5a5cca2f3cbf54b3d96db18bea3dd9`.
The +03A3 caller is file `[0x2416F,0x241A1)`, 50 bytes / 19 starts,
SHA-256
`3cef292a9346dadb89b80e93d5b78f701878140bf60bcd95aaba8d7022fc01ad`.
Its sole call is `+03AC CALL +02F2` after `+03AB PUSH CS`; its outer
`RETF` is +03D4. Verified containing call encodings to +03A3 are
`+055A E8 46 FE`, `+0813 E8 8D FB`, and `+083A E8 66 FB`.

The new focused fixture combines alias and wrap effects that the
existing C test covered separately:

1. Entry SP=FFFE, SS=2000 and DS=2923 place DS:6DCC exactly over
   saved BP at SS:FFFC. The callee returns normally; the +03A3 pending
   count clear changes the BP later restored by `POP BP` to zero. The
   outer `RETF` reads CS at separately wrapped SS:0000, not the false
   linear word following its IP at FFFE.
2. Entry SP=FFFF, SS=2000 and DS=1924 place DS:6DC2 over the high byte
   of return CS at SS:0002. One carry changes array word 2 to 1, so
   outer CS changes from 02EF to 01EF before `RETF`. IP crosses the
   FFFF physical boundary, while CS starts at wrapped SS:0001. A false
   linear CS word is ignored.

For each case two runs differ only in the false linear CS word. The
fixture compares all 14 output CPU words and every 1 MiB RAM byte
between runs, then checks the exact wrapped return, aliased words and
that writes remain in the bounded stack/array region. These are
controlled C fixtures, not original-instruction or native parity.

## Held native capture plan

The available evidence names three containing callers, but no saved-game
checkpoint or scripted input is yet known to reach one naturally.
That checkpoint and its exact input sequence are **preconditions**, not
assumed evidence. Once one is supplied and PM releases the host native
slot, pin both media hashes and the loaded U7D segment, replay the same
checkpoint/input once under a 90-second wall-clock timeout, and arm
breakpoints at the aligned +055A/+0813/+083A calls and +03A3 entry.
Capture CPU14 and 1 MiB RAM before +03A3 and at its actual outer
return, recording the +03AC/+02F2/+03AF path, return CS:IP/SP, live
SS frame words and DS:6DC2..6DCE/363A..3646 state. Compare the merged
C composition from the captured +03A3 entry with the native return over
all RAM bytes and CPU words. Preserve any transfer or nonreturn event
as such. If no breakpoint is reached by 90 seconds, stop and record
nonreach; it establishes no parity or natural-game claim. This plan
does not launch DOSBox-X while the native hold is active.

No source-only fixture changes the provisional game identity or the
strict full-C count. The reached native frame and U7D-specific wrapped
RETF native proof remain open.
