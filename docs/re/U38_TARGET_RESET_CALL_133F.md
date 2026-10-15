# Unit 0x38 entry 43: character target-reset call at +0x133F

## Scope and provenance

This unclaimed public entry is the first bounded fallback after checking the
four far-call dependencies of U38 +0x15EB. Its first U9E call reaches entry 7
at +0x0D63, which itself calls the large +0x02FA body; its second U9E call
reaches entry 10 at +0x0E52, which allocates runtime memory. The remaining
calls reach runtime `AF8:0B0B` and resident `BA:1FEB`, the latter with further
overlay calls. None offers a new, closed, small callee. The bounded local
`deja "poolrad U38 +133F entry 43 BA:2637 wrapper parity"` recall produced
no prior session result. The already reviewed
[resident `BA:2637` target reset](RESIDENT_CHARACTER_TARGET_RESET.md) supplies
the callee behavior and character-record caller role used here.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

U38 public index 43 alone enters `[+0x133F,+0x1355)`, `GAME.OVR` file
`[0xFF9A,0xFFB0)`: 22 contiguous bytes, nine instructions, SHA-256
`f5d32541e8f8b2ce0e4d08ed887a5ddf9c52829f449f74b8c1553a6b0bbfe8ce`.
The only loader fixup is the segment word at `+0x134B` of `LCALL BA:2637`
at `+0x1348`. `BA` is the file placeholder; the call uses the loaded word.
The return is `RETF 10` at `+0x1352`. The source model is disjoint from the
resident callee and other U38 entries.

## Exact wrapper behavior

The entry saves BP, then pushes caller words `[BP+0x0E]` and `[BP+0x0C]`
as the far character-record pointer. It calls the loaded resident
`BA:2637` target-reset function. That separately verified full-C routine
performs four ordered byte clears through the record's `+0x108` far link,
returns AL=1 while preserving AH, and exits `RETF 4`. The wrapper applies
`OR AL,AL` to set flags; its `JE +0x1351` and fall-through both arrive at
the same `POP BP`. It exits `RETF 10`. The model preserves the resident
callback's full RAM and CPU effects, including its actual stack frame;
callback refusal leaves the CPU at the resident entry.

The resident review documents 33 fixup-backed game callers, including U38
`+0x1348`, and the character-record role. This supports game-specific
ownership of the wrapper, subject to PM classification review. It does not
identify a reached gameplay caller frame for this entry.

## Verification and gate

`tests/recovered/check_overlay_u38_target_reset_call_133f.py` checks the
input hashes, entry alias, exact extent, fixup, instruction coverage, and
far target. It executes the original 22 bytes in Unicorn and intercepts
only the loaded resident call. Twenty-four cases compare all 1 MiB RAM,
14 CPU fields, and the five-word callback frame. They vary AL=0/1/0x80/0xFF,
incoming AX, loaded segment, DS/SS alias, and a stack push across offset
zero. The callback models a deterministic resident return with `RETF 4`.

Four DOSBox-X normal-core synthetic far-entry frames execute the original
wrapper and a deterministic resident stub. All 14 CPU words, 52 stack
bytes, and `DS:6816` match C. The COM, original-byte copy, log, and capture
remain under ignored `resources/u38-133f-parity/native/`; result SHA-256:
`ca5c5a72afe6af8e2f310ad79b13eefb713df834e4aebf9d4fbd443786f2b638`.
The focused C11 `-Wall -Wextra -Wpedantic -Werror` ASan/UBSan CTest passes,
including both AL branch outcomes and callback abort.

The native target is synthetic and no reached game caller frame has been
captured. The wrapper's complete C behavior is verified against original
bytes at this boundary; strict identity and count credit remain for PM review.
