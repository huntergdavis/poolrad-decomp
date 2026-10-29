# U9E entry8 +0E05 loopback to the next SYSTEM call

This is the bounded successor after the published +0E13 post-SYSTEM
JNE takes its loop edge. It models original `+0E05..+0E0D` and stops
before the separate +0E0E SYSTEM far-call adapter. It neither composes
entry8 nor executes its shared tail or `RETF 4`. PM assigned this B
scope after PR #682 (i7782). The bounded `deja` query for this exact
loopback timed out without a result; the adjacent checked-in
[callback map](U9E_ENTRY8_FULLSTATE_CALLBACK_MAP.md),
[postcall](U9E_ENTRY8_FULLSTATE_POSTCALL_0E13.md), and
[SYSTEM adapter](U9E_ENTRY8_SYSTEM_CALL_0E0E.md) supplied the frame
interface, which was checked against original bytes again.

Ignored owner media `GAME.OVR` is SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit U9E file base is `0x2AEEA`; full entry8 is
`[0x2BCCD,0x2BD18)`, 75 bytes / 29 starts, SHA-256
`7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba`.
This slice is file `[0x2BCEF,0x2BCF8)`, exactly nine bytes:
`[scrubbed: span=U9E_ENTRY8_FULLSTATE_LOOP_0E05-1 sha256=dc1422933023de40130adfaddf73ed03e684a443f89d39b69fc4f3c27d1b107d bytes=9 offset=UNRESOLVED]`, SHA-256
`dc1422933023de40130adfaddf73ed03e684a443f89d39b69fc4f3c27d1b107d`.
The original `+0E1D JNE` targets +0E05; the next five bytes are the
published `+0E0E LCALL 0AF8:11C0`. The shared tail ends with
`+0E2B RETF 4`; +0E2D is its second cleanup-immediate byte, not an
executable start.

At +0E05 the live postcall CPU14 has BP and SS locals. `INC BYTE
SS:[BP-2]` updates the loop index; `MOV AL,SS:[BP+6]` rereads the
caller range from live RAM, including any prior SYSTEM seed alias;
`XOR AH,AH` clears AH and defines CF=OF=SF=0 and ZF=PF=1 because the
**byte AH** result is zero. AF is undefined for XOR; following the
adjacent source convention, the C result retains the AF produced by
the preceding byte INC, including a low-nibble carry. `PUSH AX` then
writes a physically contiguous WORD at SS:SP-2, where the SS offset
wraps before A20-off physical address calculation. That push can alias
the DS seed; the next SYSTEM adapter must see its changed live RAM.
The slice stops at +0E0E with SP decremented by two.

The C API rejects wrong arguments/IP and writes into the pinned loaded
U9E body unchanged. Focused strict C11/ASan/UBSan cases compare all
1 MiB RAM bytes and 14 CPU words for AF carry and no carry, a PUSH
word crossing physical FFFFF, and a DS seed alias. The static checker
pins media/body/slice hashes and the JNE, LCALL and `RETF 4` boundaries.
These controlled source cases are not original-instruction or native
parity. B0:0048 dispatch, full entry8 composition, a natural U7F
caller frame and strict full-C count credit remain open.
