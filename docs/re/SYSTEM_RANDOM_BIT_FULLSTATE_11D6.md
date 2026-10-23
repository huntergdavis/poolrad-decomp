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

`AND DH,0x7F` defines CF/OF/SF/ZF/PF; architectural AF is undefined. The source
clears AF to match the pinned DOSBox-X AND implementation. Unsupported arguments or
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

## Declared RETF oracle split

On main `1354e06ab253594df12802f5204f21def4f0ba28`, the first three
original-byte cases passed, but `wrap-fffe` failed with "original did not
reach C's transfer target". An independent return hook isolated the cause:
Unicorn 2.1.4 reads RETF's CS word contiguously beyond the stack segment at
entry SP FFFE/FFFF. The C model already reads it at the separately wrapped
16-bit stack offset. Original +11D6 bytes and a synthetic single-byte RETF
both produced these results with SS=2000:

| Entry SP | Wrapped CS word address/value | Contiguous CS word address/value | Unicorn CS:IP / final SP |
| --- | --- | --- | --- |
| 8000 | 28002 / CAFE | 28002 / CAFE | CAFE:BEEF / 8004 |
| FFFE | 20000 / CAFE | 30000 / A5A5 | A5A5:BEEF / 0002 |
| FFFF | 20001 / CAFE | 30001 / A5A5 | A5A5:BEEF / 0003 |

This reuses the measured stack-offset rule and VP decision i19555 documented
in [RES_0709_0714](RES_0709_SKIP_SPACES_0714_CPU.md#stack-offset-edges-the-defect-and-its-fix),
located through Dunking Sheep memory and a widened source/evidence search.
PM instruction i22316 applies that rule here: C follows pinned DOSBox-X;
Unicorn's difference is asserted explicitly, never broadly masked.

The pinned DOSBox-X source revision is
`5a9c7c53481c2e933948039c33256776653f4faf`; the corresponding local build
has SHA-256 `c05ce28a60258c320c5a209a477603d91fb2b7558c0cc03919e518ddd99eb457`.
In that instrumented source tree, `src/cpu/cpu.cpp:2234–2235` implements
16-bit real-mode `CPU_RET` with two successive `CPU_Pop16()` calls.
`CPU_Pop16`, lines 697–704, reads a word at `SS + (ESP & stack.mask)` and
updates ESP with `(ESP + 2) & stack.mask` before the next pop. With a 16-bit
stack, the second word therefore starts at SS:0000/0001 for SP FFFE/FFFF.
The bytes within each word remain physically contiguous. This is source
verification against the pinned build, not a new native +11D6 execution.

The checker now stops after the original RETF, or after a redirected seed
RET, independently of the candidate's return target. Only `wrap-fffe` at
entry SP FFFE and `wrap-ffff` at entry SP FFFF permit a CS difference. Each
asserts that the two fixture words differ, C's CS equals the wrapped word,
and Unicorn's CS equals the contiguous word. All 1 MiB of RAM and the other
13 CPU words, **including full FLAGS**, must be equal. The other five cases,
including the SP 8000 controls, compare all 14 CPU words exactly. The old
blanket AF mask is removed.

For the negative control, the checker compiles a temporary copy of the C
model with only the final CS read changed to the contiguous location. Both
wrap cases must reject that mutant specifically at the C wrapped-CS
assertion. The production C source is unchanged. This repair adds no
parity credit and does not extend the bounded seven-case contract.
