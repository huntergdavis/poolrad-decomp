# Unit 0x38 public entry 55: roll and dispatch at +0x1699

## Provenance and ownership

This is the smallest unclaimed grade-A, game-evidenced U38 leaf in the
current classification ledger. It was selected after the adjacent
+0x2BC8 entry because the complete +0x0FAC callee's four loaded far
calls remain unbounded. The bounded `deja "poolrad U38 +1699 leaf
original bytes callers source parity ownership"` query timed out
after 20 seconds without a session result. This candidate reuses the
measured [U9E entry-8 roll ABI](U9E_ENTRY8_ROLL.md) (`RETF 4`) and
independently checks the [+0x15EB](overlay_function_ledger.csv)
helper's `RETF 8` against original bytes.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

The complete public entry is U38 `[+0x1699,+0x16BB)`, `GAME.OVR`
file `[0x102F4,0x10316)`: 34 contiguous bytes / 17 instructions,
SHA-256 `26dcadfce0b6eea4f5c1f898eb4eae5ddba59c48914064cf0c9a946f1dcf14ff`.
Only public index 55 aliases +0x1699. The wrapper has one loader fixup,
at `+0x16AB`, the segment word of its far call at +0x16A8. The sole
near call is at +0x16B4 to +0x15EB. That nonpublic helper is a separate
114-byte body, SHA-256
`43cff4c17d026213e5b7fbc0ffeb0a549c481931bd8f3919dd92cf8e87f5444c`;
it has four further far-call fixups and exits by `RETF 8`.

## Exact wrapper and callback boundary

The wrapper pushes caller words `[BP+0x0E]` and `[BP+0x0C]`, then pushes
two WORDs whose low bytes are 2 and 8. Their high bytes retain the
incoming `AH`. It calls the loaded U9E roll target at `:0048`, using
the segment word at `CS:16AB`. The roll's `RETF 4` consumes the two
newest WORDs and leaves the two caller words on the stack.

The wrapper clears `AH` with `XOR AH,AH`, pushes the zero-extended roll
result, pushes zero, then `CS`, and near-calls U38 +0x15EB. That
helper's `RETF 8` consumes those two words plus the two caller words
left by the roll. The wrapper then restores `BP` and exits `RETF 10`.
`pr_u38_roll_dispatch_1699` models all 34 bytes in 1 MiB RAM and the
14-field CPU state. Its two callbacks see the actual call-entry
registers and stack. A refused callback returns with its frame intact.

## Verification and gate

`tests/recovered/check_overlay_u38_roll_dispatch_1699.py` executes
the original wrapper bytes and intercepts only the loaded roll target
and +0x15EB. Forty-eight cases compare the entire 1 MiB RAM, all 14
CPU fields, and both six-word callback frames. They vary roll result,
incoming `AH`, loaded fixup segment, DS/SS alias, and stack wrap.
The focused C11 `-Wall -Wextra -Wpedantic -Werror` ASan/UBSan CTest
covers four roll results and both callback abort sites.

Four DOSBox-X normal-core synthetic far-entry frames execute the
original wrapper with deterministic roll and +0x15EB stubs. All 14
CPU words, 52 stack bytes, and `DS:680F/6818` match C. The COM,
original-byte copy, log, and capture stay under ignored
`resources/u38-1699-parity/native/`. Capture SHA-256:
`41608ecd013fe1fd4b917942bea7f734fd245e7dfe5dc14b911975dbcba9b5d9`.

The native call targets are synthetic and no reached game caller frame
exists. +0x15EB's complete effects are not modeled here. This is a
provisional full-wrapper candidate with no strict identity or count
claim.
