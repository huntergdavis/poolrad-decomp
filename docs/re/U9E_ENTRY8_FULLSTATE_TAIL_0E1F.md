# U9E entry8 shared far-return tail

`pr_u9e_entry8_fullstate_tail_0e1f` translates only U9E entry8
`+0E1F..+0E2D`. It accepts supplied live 1 MiB RAM and CPU14 at the
shared tail, executes the final local-byte copy and `RETF 4`, then returns
the caller's CS:IP and stack state. It does not execute the preceding
loop, nested SYSTEM call, or B0:0048 dispatch. PM assigned this as the
separate B gate after the +0DE3 prefix and +0E13 post-call A (i7580).
The exact `deja "U9E entry8 0E1F RETF4 tail stack wrap"` query found no
indexed prior session; the checked-in [entry8 callback map](U9E_ENTRY8_FULLSTATE_CALLBACK_MAP.md)
supplies the frame ABI, which was checked again against ignored media.

Ignored DOS v1.3 `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The complete U9E entry8 body is file `[0x2BCCD,0x2BD18)`, 75 bytes / 29
starts, SHA-256
`7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba`.
This tail is file `[0x2BD09,0x2BD18)`, 15 bytes / six starts, SHA-256
`480f3e495cbb8f3c29ac48422545fe45f39c92e58c01da4b5aa254d03e1a9c42`.
Its original instructions are `MOV AL,[BP-3]`, `MOV [BP-1],AL`,
`MOV AL,[BP-1]`, `MOV SP,BP`, `POP BP`, and `RETF 4`.

Each effective SS offset wraps to 16 bits before the real-mode physical
address is formed. A word whose first byte is at SS:FFFF reads its second
byte at the next physical address; the next *word* begins at its separately
wrapped effective offset. In particular, BP=FFFC places the return IP at
FFFE and the return CS at 0000. BP=FFFD places IP at FFFF and CS at 0001.
The final SP is BP+10 modulo 16 bits. No instruction in this tail changes
FLAGS. The source rejects a write alias into the pinned loaded entry8 body
before mutation and assumes that body was loaded from original bytes.

The focused C11 test checks normal and wrapped return frames, false linear
CS decoys, exact 1 MiB RAM and CPU14 state, and unchanged unsupported input.
A separate manual static audit pins original and loaded bytes and exercises
all three frames without an emulator. Native full-state parity and natural
reach remain open under the host hold. This bounded source gate carries no
native, complete-function, or strict full-C count credit.
