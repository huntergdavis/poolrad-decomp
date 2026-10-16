# SYSTEM +11C0 full-state random wrapper candidate

`pr_system_random_fullstate_11c0` translates the complete local SYSTEM
`+11C0..+11D5` body and composes its only near call with the merged
[`+120F` seed-step source](SYSTEM_SEED_FULLSTATE_120F.md). It operates
on the same live 1 MiB RAM and 14 CPU words. It is separate from the
adjacent `+11D6` routine, the U9E entry8 callback, and the wider ECL
RANDOM chain. This conditional source candidate adds no native or strict
full-C credit.

The bounded `deja` query for SYSTEM `+11C0` full-state range, DIV, and
`RETF 2` parity found no indexed exact session. This candidate reuses
the pinned SYSTEM span and U9E far-call mapping from the checked-in
[entry8 audit](U9E_ENTRY8_ROLL.md) and
[callback ownership map](U9E_ENTRY8_FULLSTATE_CALLBACK_MAP.md). The
source bytes were checked again from ignored owner media.

## Original extent, callers, and frame

Ignored DOS v1.3 `START.EXE` is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The wrapper is file `[0xBE36,0xBE4C)`, 22 bytes / 10 contiguous
instructions, SHA-256
`e4337c9ed94c3d0a72864fcf23dd6e8fb30693d25680a1d52401c919fcbbfeb9`.
It begins with near `CALL +120F` at `+11C0`, branches at `+11CD`, and
ends in `RETF 2` at `+11D3`. The nested `+120F` body is file
`[0xBE85,0xBEBB)`, 54 bytes / 22 instructions, SHA-256
`f5b67b3eec594ce9fa18e9d78e0bb2eec93c9475a758f2c7ae9988ad00ce6242`.
The U9E entry8 `+0E0E` far-call fixup and U1C `+0266` are known callers
of `+11C0`; this note does not enumerate the p5-owned ECL RANDOM chain.

At wrapper entry, SS:SP holds the far caller's return IP, SS:SP+2 its
CS, and SS:SP+4 a range WORD. Each **offset** addition wraps at 16
bits before the A20-off physical byte address is formed; the two bytes
within a WORD are physically contiguous. `CALL +120F` first pushes
near return IP `+11C3` at SS:SP-2. The seed step may mutate that word:
if its live near `RET` reaches another IP, this API stops with an
explicit callee-transfer status and leaves RAM/CPU state at that point.

After a normal seed return, `+11C3` clears AX, takes BX from SP, then
loads the range from **live** `SS:[BX+4]`. That read occurs after the
ordered DS:4428/442A seed writes, so a seed/range alias can change BX.
The `OR BX,BX` zero branch returns AX=0 while retaining the new seed's
high word in DX. For nonzero BX, `XCHG DX,AX; DIV BX; XCHG DX,AX`
returns `high16(seed) % range` in AX and the quotient in DX. DX is zero
at DIV, so a nonzero WORD divisor cannot overflow. `RETF 2` then reads
return IP and CS from separately wrapped SS offsets, consumes the range
WORD, and advances SP by six.

The last defined arithmetic FLAGS come from `OR BX,BX`: CF/OF clear,
SF/ZF/PF reflect BX. AF is undefined for OR, and DIV leaves arithmetic
FLAGS undefined; the source preserves those undefined bits from the
seed-step result rather than claiming their original native values.
Original-byte/native CPU14 parity is still required before any strict
accounting decision.

## Contract and focused checks

The API rejects null pointers, wrong RAM size, wrong entry IP, and a
near-call or seed write that physically overlaps either pinned SYSTEM
instruction body before any mutation. It otherwise assumes the loaded
CS:`+11C0..+11D5` and CS:`+120F..+1244` bytes are the original code.
The multiplier at live CS:`+1245` remains mutable and is read by the
composed seed step. Stack/seed/range aliases outside executable code
retain original source order.

Focused C11 cases check both range branches, exact quotient/remainder,
the 1 MiB RAM diff including the visible near-call word, all 14 CPU
words, a seed/range alias, an altered near-return transfer, outer RETF
offset wrap at SP FFFE/FFFF, and unchanged unsupported input. The
existing address-only audit pins both SYSTEM spans and the U9E fixup.
No new Unicorn or DOSBox-X work ran under the host native hold.
