# SYSTEM +120F full-state seed step candidate

`pr_system_seed_fullstate_120f` translates the complete local SYSTEM
`+120F..+1244` seed-step body using live A20-off RAM and 14 CPU words.
It is a separate candidate from the `+11C0` random wrapper and the ECL
RANDOM chain. It has no strict full-C or reached-native credit yet.

The bounded `deja` query for the U9E entry8 full-state callback and
SYSTEM `+11C0/+120F` found no matching indexed session. This candidate
reuses the original-span and seed-algebra evidence in the checked-in
[U9E entry8 audit](U9E_ENTRY8_ROLL.md) and the
[callback ownership map](U9E_ENTRY8_FULLSTATE_CALLBACK_MAP.md); the bytes
and call sites below were checked again from ignored owner media.

## Original extent and callers

Ignored DOS v1.3 `START.EXE` is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
SYSTEM `+120F..+1244` is file `[0xBE85,0xBEBB)`, 54 bytes / 22
contiguous instructions, SHA-256
`f5b67b3eec594ce9fa18e9d78e0bb2eec93c9475a758f2c7ae9988ad00ce6242`.
It makes no call and ends in near `RET` at `+1244`. Its immediately
following CS word at `+1245` is `0x8405` in the original image; the C
candidate reads this operand from live `CS:1245` RAM.

The distinct SYSTEM `+11C0..+11D5` wrapper is file `[0xBE36,0xBE4C)`,
22 bytes / 10 contiguous instructions, SHA-256
`e4337c9ed94c3d0a72864fcf23dd6e8fb30693d25680a1d52401c919fcbbfeb9`.
Its first instruction is a near `CALL +120F`, and it ends in `RETF 2`.
The adjacent `+11D6..+11F2` 29-byte body also decodes contiguously from
a near `CALL +120F` to `RETF`. A raw `E8` encoding at `+11F3` points to
`+120F`, but its containing function boundary is unresolved, so it is a
candidate caller only. The known `+11C0` far callers include U9E entry8
`+0E0E` and U1C `+0266`; broader ECL RANDOM fan-in belongs to its
separate owner. This caller screen does not claim exhaustive reachability.

## State and return contract

The API enters at `CS:120F` with exactly 1 MiB RAM. It rejects a null
pointer, different RAM size, or different IP before any mutation. It
also rejects a DS seed byte that physically overlaps any byte of the
CS:`+120F..+1244` instruction body, before any mutation. The API can
detect this address alias, but cannot verify that arbitrary nonaliasing
code RAM still holds the pinned original instructions. Immutable original
code bytes are therefore an explicit precondition. The adjacent
CS:`+1245` multiplier WORD is intentionally read live and is not part
of that rejection span. The routine
reads DS:4428, DS:442A, then the live CS:1245 multiplier WORD in source
order. The 16-bit MUL and subsequent shifts, additions, and byte-high
additions update AX, BX, CX, and DX. The final `ADD AX,1; ADC DX,0`
sets the returned arithmetic FLAGS; other FLAGS bits and SI, DI, BP,
CS, DS, ES, and SS are preserved. The resulting seed agrees with
`seed * 0x08088405 + 1` modulo 2^32 when the CS operand is the original
`0x8405`, but the API does not replace the live operand with a constant.

The routine writes AX to DS:4428, then DX to DS:442A. Its near `RET`
reads IP from **live** SS:SP after both writes and advances SP by two
modulo 65536. Each word's bytes are physically contiguous with A20
disabled. If DS seed memory aliases the stack return word, the new seed
changes the destination. This boundary makes no claim about `+11C0`'s
argument frame, DIV behavior, or far return.

## Verification and remaining gate

Focused strict C11 cases check all 1 MiB bytes outside the seed writes,
all CPU14 words, a non-original live multiplier, a seed/return alias,
wrapped SP, code alias and invalid inputs without mutation, and carry/overflow cases
for the final ADC. The existing address-only U9E entry8 audit pins the
SYSTEM spans and U9E far-call fixup. The full original-byte and native
RAM/CPU14 parity gate remains open while the host native hold is active;
no DOSBox-X or new Unicorn run was launched for this candidate.

The next disjoint `+11C0` candidate will call this API on the same live
RAM/CPU state, then model its range read, zero branch, DIV, and `RETF 2`.
Neither candidate alone closes U9E entry8 or the U7F callback.
