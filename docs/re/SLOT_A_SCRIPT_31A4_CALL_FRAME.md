# Slot-A U1C +31A4 dispatcher call frame

PM inbox i7409 approved a bounded source model for `PUSH CS` and near
`CALL`, stopping at dispatcher entry. A targeted `deja` query for this
frame found no prior session match. The merged +3196 gate supplies the
explicit CPU/RAM preflight pattern. Checked-in p9 evidence in
`SLOT_A_DELAY_RESIDUAL_CALLER_AND_REPLAY.md` is reused only as an
indirect downstream stack-frame check. The broad high-level
`ecl_helpers.c` runner maps the call but does not own this exact frame.

DOS v1.3 GAME.OVR file 0x3EEA..0x3EED and saved loaded RAM physical
0x389C4..0x389C7 both contain `0E E8 85 FD`, SHA-256
`778ab0e467c211ff7983557c9aa5b3917ecefb983bb2f59f7e0da05c8fe59179`.
The audit pins complete GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`
and saved +33BD RAM SHA-256
`38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3`.
`objdump -m i8086` decodes `PUSH CS` at +31A4 and near `CALL` with
displacement FD85 at +31A5. Relative to return IP +31A8, the target
is +2F2D. Saved RAM pins loaded code, not a native +31A4 stop.

The runner prologue at +3171 pushes BP and sets BP=SP. A supplied
first-visit frame with caller SP=3EF0 therefore has SP=BP=3EEE at
+31A4. With SS=1D16, `PUSH CS` writes 3582 to SS:3EEC (A20 physical
0x2104C..0x2104D), then `CALL` writes return IP 31A8 to SS:3EEA
(0x2104A..0x2104B). The stop at 3582:2F2D has SP=3EEA, BP=3EEE,
all other registers and flags unchanged. p9's downstream residual
return capture found SS:3EEA=31A8 and SS:3EEC=3582, then returned
to +31A8 with SP=3EEE. That corroborates this frame indirectly; it
does not establish a direct first-visit +31A4 stop.

`pr_rec_u1c_31a4_call_dispatch_frame` accepts supplied ZF-set CPU at
3582:31A4 and complete 640 KiB or 1 MiB RAM. It checks the four
opcode bytes, preflights both A20-masked stack words before mutation,
writes CS then return IP in instruction order, and stops before any
dispatcher effect. Word bytes are contiguous physically, including
across offset FFFF. Rejected inputs leave CPU and RAM unchanged.
Focused tests compare all 14 CPU words and complete RAM for both sizes,
the first-visit frame, SP/A20 wrap, an operand crossing the 640 KiB
boundary, code/stack alias, four wrong opcode bytes, and bad
flag/address/size/null inputs. The tests are synthetic and do not
establish natural +3171 reach. Strict C11, ASan/UBSan, source audit,
and CTest registration are local checks; no native or full build is used.

## Parity review

- **target_function:** U1C +31A4..+31A7, stopping at 3582:2F2D.
- **source_candidates:** `pr_rec_u1c_31a4_call_dispatch_frame`.
- **signals:** exact original/loaded bytes, i8086 displacement,
  downstream p9 frame words, synthetic full-state tests.
- **score and verdict:** 2/5 direct signal groups; **unclear** for a
  natural first-visit route, source-only conditional instruction match.
- **review_notes:** no direct +31A4 stop, dispatcher effects, +31A8
  successor, full-function, or gameplay credit is claimed.
