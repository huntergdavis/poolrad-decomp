# Unit 0x38 entry 40: active-character string comparison at +0x1276

## Provenance and selection

This is a disjoint, public, game-evidenced candidate after the U38 +0x15EB
dependency graph produced no new small closed game callee. The exact prior
`deja "poolrad U38 +1276 active character AF8:0C10 compare full C parity"`
query timed out after 20 seconds without a session result. The comparator
identity and string behavior reuse the independently decoded
[SYSTEM `AF8:0C10` evidence](RESIDENT_STRING_0356.md) and its
[reached unit-0x19 frame](NATIVE_COLD_START_U19_POST_RETURN_COMPARE.md).
This branch independently rehashed and executed the original comparator.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

The sole public index 40 enters U38 `[+0x1276,+0x12B2)`, `GAME.OVR` file
`[0xFED1,0xFF0D)`: 60 contiguous code bytes / 24 instructions, SHA-256
`0598c5bb03bf995e3aeba41c11d749c629fa65cd273bd6a00892bf9a5eaca01a`.
Its only loader fixups are `+0x1293` and `+0x12A5`, the segment words of
far calls at `+0x1290` and `+0x12A2` to SYSTEM `:0C10`. The 14 adjacent
constant bytes at U38 `[+0x1268,+0x1276)` hash to
`eb19eb8681d04b84bfe569f341ba3b2de215ff19ee7c51282198074a48c57ae4`;
their contents stay in ignored owner media. They belong to the preceding
public span and are only read here.

The hash-locked START loader expands SYSTEM `AF8:0C10` to image
`[0x0BB90,0x0BBBB)`, 43 original bytes, SHA-256
`11e2860118cb471f9f1ecb1d0c44c060e0fbb83862f69c1cf39b5e0484298a58`.
The C model composes this body rather than stopping at an opaque callback.

## Complete behavior

The wrapper compares the active character record byte `+0x9F` through
DS `[0x5D92]/[0x5D94]` with one. If it differs, it returns. Otherwise it
passes that same far pointer and first the `CS:1268` Pascal constant to
the loaded `:0C10` comparator. If the first comparison is unequal, it
reloads the active pointer and compares against `CS:1270`. Equality of
either string subtracts four from DS byte `[0x6822]`; both unequal leaves
that byte alone. The wrapper exits `RETF 10`.

The composed comparator executes its original `CLD`, loads the two far
pointer arguments from the actual SS call frame, compares Pascal lengths
and then up to the shorter length with `REPE CMPSB`, restores DS, and exits
`RETF 8`. It preserves the original register results: BX is call-entry SP,
DX is the caller DS, SI/DI advance through compared bytes, CX retains the
remaining comparison count, AX holds the two length bytes, and ES holds the
second string segment. Defined arithmetic flags and DF match the original
bytes. Both loader segment words are read from RAM, so they can differ.

The existing classifier marks +0x1276 game-evidenced because it reads the
active-character pointer. No fixup-backed direct caller or reached game
frame for this public entry is known; its game ownership remains provisional.
A raw `76 12` word at U38 +0x1887 is a `JBE` instruction, not a procedure
pointer to +0x1276.

## Verification and gate

`tests/recovered/check_overlay_u38_character_compare_1276.py` checks both
input hashes, public entry, exact extent, two loader fixups, all U38
instructions, adjacent constant hash, and the expanded SYSTEM hash. It
runs both original bodies together in Unicorn. Twelve cases compare all
1 MiB RAM and 14 CPU fields. They cover marker miss, first equality,
second equality, both unequal, empty and shorter strings, two loaded
segments, DS/SS alias, and incoming DF set.

Four DOSBox-X normal-core synthetic far-entry frames execute the original
wrapper, constants and SYSTEM comparator bytes, with no comparator stub.
All 14 CPU words, 52 stack bytes and DS `[0x6822]` match C. The COM,
original-byte copies, log, and capture stay under ignored
`resources/u38-1276-parity/native/`; result SHA-256
`e765547efa06fc2083d44d515f29dee8753e2121a663c179be73f37970aefa7a`.
The focused C11 `-Wall -Wextra -Wpedantic -Werror` ASan/UBSan CTest passes.

The native frames are synthetic, not reached game callers. This branch
makes no strict identity or count claim before PM ownership review.
