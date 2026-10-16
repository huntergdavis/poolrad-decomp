# SYSTEM +11D6 full-state random bit candidate

`pr_system_random_bit_fullstate_11d6` translates the complete local SYSTEM
`+11D6..+11F2` body on live 1 MiB RAM and CPU14. It composes the merged
[`+120F` seed step](SYSTEM_SEED_FULLSTATE_120F.md) and is distinct from the
merged [`+11C0` range wrapper](SYSTEM_RANDOM_FULLSTATE_11C0.md). The bounded
`deja` query for this exact SYSTEM +11D6 body found no indexed prior session;
the same original START.EXE mapping and seed API used by those two checked-in
notes provide the starting ABI, and all +11D6 bytes were checked independently.

## Original extent and behavior

Ignored DOS v1.3 `START.EXE` is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The wrapper is mapped image `[0xC156,0xC173)` and contiguous file
`[0xBE4C,0xBE69)`: 29 bytes, 13 complete instructions, SHA-256
`2fdaa8058f09e137bb95255831266138e17e594a39502eee7e515172cefd01ff`.
Its near `CALL +120F` begins at `+11D6`; `RETF` ends at `+11F2`. The
composed seed body remains the independently pinned 54-byte/22-instruction
`+120F..+1244` span (SHA-256
`f5b67b3eec594ce9fa18e9d78e0bb2eec93c9475a758f2c7ae9988ad00ce6242`).

The call first pushes near return IP `+11D9` at live SS:SP-2. The seed step
writes DS:4428/442A in source order and reads that live near return word. If
the seed write changes the return destination, this wrapper returns an explicit
callee-transfer status and leaves the resulting RAM/CPU state at that point.
Otherwise `XCHG BX,AX` places the new low seed in BX. AX becomes `0x0080`,
CX becomes 32, and each loop checks DH bit 7. An unset bit shifts BX left,
rotates DX left through BX's carry, decrements AL, and decrements CX. A set
bit jumps to `AND DH,0x7F`. If all 32 iterations expire, `XOR AL,AL` clears
the result before the same AND. The final `RETF` reads caller IP and CS at
separately wrapped SS offsets and advances SP by four.

`AND DH,0x7F` defines CF/OF/SF/ZF/PF; AF is undefined. The source preserves
AF from the seed result and models the defined bits. Unsupported arguments or
a near push/seed write into either pinned instruction body reject unchanged.
Loaded instruction bytes are assumed to be the originals; the live multiplier
at CS:+1245 remains mutable.

## Evidence gate

The focused C11 test covers an immediate high-bit exit, 31 shifts from a
zero seed, full 1 MiB RAM effects, CPU14 state, a redirected near return,
wrapped outer RETF frames, and unchanged unsupported input. The separate
`check_system_random_bit_fullstate_11d6.py --exe <ignored START.EXE>` harness
pins the executable, mapped file provenance, 13/22 instruction extents, and
compares seven original-byte Unicorn cases against C, including aliases and
frame wrap. It is a manual gate because owner media and a scheduled native
slot are required; its result must be recorded before any parity or strict
full-C credit claim. The wider ECL RANDOM chain and callers are outside this
candidate.
