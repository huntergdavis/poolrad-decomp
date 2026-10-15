# Unit 0x38 public entry 84: conditional roll effect at +0x27D9

## Provenance and extent

The bounded `deja "poolrad U38 +27D9 entry84 +2800 +280E strict C parity"`
query found no prior-session match. This work reuses the verified
[U9E entry-8 roll ABI](U9E_ENTRY8_ROLL.md) and the checked
[U38 roll callback pattern](U38_ROLL_DISPATCH_1699.md). Entry extent,
fixups, and both loaded segment words were independently checked against
ignored local inputs:

| Input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

Public entry 84 is the complete unit `0x38:[+0x27D9,+0x2814)`,
`GAME.OVR` file `[0x11434,0x1146F)`: 59 bytes / 28 contiguous
instructions, SHA-256
`8247af44aa38ad460b80fe1491657b6a84b0228d483b706c4f91f3664180aee5`.
The next public entry starts at `+0x2814`. The two loader fixups in
this span are `+0x2800` and `+0x280E`, the segment operands of far
calls at `+0x27FD` and `+0x280B`. The original immediate `0xB0` is
not treated as the runtime segment.

## Behavior and callback boundaries

The entry returns immediately if byte `DS:6819` has bit 0 or bit 4 set.
Otherwise it pushes caller words `[BP+0x0E]` and `[BP+0x0C]`, then
three words with low bytes `0x66`, 3, and 6 while retaining incoming
`AH`. It calls the segment loaded at `CS:2800`, offset `:0048`.
The U9E roll's `RETF 4` consumes the last two argument words, leaving
three words for the later call. The wrapper clears `AH`, pushes the
zero-extended roll result, then words `0xFF` and 1, and calls the
independently loaded segment at `CS:280E`, offset `:0052`. Original
U9E entry 10 at `+0x0E52` ends with `RETF 12`, consuming all six
remaining words. The wrapper returns with `RETF 10`.

`pr_u38_roll_effect_27d9` models all wrapper instructions with 1 MiB
RAM and the 14-field CPU state. Separate callbacks receive the exact
far-call entry frames and supply each callee's register, flag, and RAM
effects. Callback refusal leaves the pending frame intact. The C model
reads both loaded segment words from code RAM; they may differ.

## Measured parity and limits

The original-byte Unicorn checker patches only the two fixup segment
words and executes the pinned wrapper. Across 192 cases it compares
the complete 1 MiB RAM, all 14 CPU fields, and both callback entry
frames. Cases cover both early-return bits, roll bytes 0, 1, 0x7F and
0xFF, incoming `AH` values, two distinct loaded segment pairs, and
three DS/SS/stack layouts.

Four DOSBox-X normal-core synthetic far-entry frames execute the pinned
wrapper with separate loaded-segment stubs for `:0048` and `:0052`.
They compare all 14 CPU words, 52 stack bytes, and `DS:6819/6818`.
The native capture stays under ignored `resources/u38-27d9-parity/native/`,
SHA-256
`d74b533645f2a064573f0a0e0dd4a7a8364105ca79011c1d78e772d7eb501ed8`.
Focused C11 `-Wall -Wextra -Wpedantic -Werror` ASan/UBSan CTest covers
both early returns, two roll results, and refusal at each callback.

These are bounded wrapper and synthetic dependency checks. The real
U9E callees are not composed here, and no natural game caller frame
has been reached. Game identity and strict function count remain
provisional.
