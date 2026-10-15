# Unit 0x38 public entry 104: linked-target message at +0x2BC8

## Provenance and exact extent

This is the separate adjacent follow-on after public entry 103. The
bounded `deja "poolrad U38 +2BC8 entry104 strict C original extent
callers parity"` query timed out after 25 seconds without a session
result. The earlier [+0x101D selector audit](U38_TARGET_SELECT_101D.md)
provided the active character's `+0xCC/+0xCE` link interpretation;
this entry's bytes, branch path, and call frame were independently
verified. No +0x101D code is composed here.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

U38 public entry 104 is `+0x2BC8..+0x2C1C`, `GAME.OVR` file
`[0x11823,0x11877)`: 84 bytes, SHA-256
`3814d2a3bd6e04f11ffe90f79dd045db903a7465d1c5bd8198ea9ffe84767538`.
The reachable code is only `+0x2BC8..+0x2C11`: 73 bytes / 26
instructions, SHA-256
`10817b3e62b587de5facc5278c0fc6bd0e09fe0eb53a80e2c445d795582ac5fe`.
The remaining 11 bytes before entry 105 are trailing data, SHA-256
`84fe9ec3307cb912bd26e9dafdd2b0c423fe57ae895878ffbb486064048fd167`;
they are not translated as instructions. Entry 104 is the sole public
alias at +0x2BC8; no loader fixup lies in its public span, and no
decoded near call from another public U38 entry targets it.

## Behavior and dependency

The entry reads the active character pointer at `DS:5D92/5D94`, then
its far link at character offsets `+0xCC/+0xCE`. A null link returns
without a call. A nonnull link whose byte `+0x2E` equals `0x57` or
`0x58` pushes caller words `[BP+0x0E]` and `[BP+0x0C]`, sets `AL=0x32`
while retaining `AH` from the link-word OR, pushes `CS`, and near-calls
`+0x0FAC` at `+0x2C08`. The call entry has return IP `2C0B`, the
pushed CS, and three argument words; the callee uses `RETF 6`.
The wrapper then exits by `RETF 10` for every branch.

The +0x0FAC callee is a distinct 113-byte provisional helper ending
at +0x101D, with four loader fixups. It has no recovered full-C
model. `pr_u38_link_message_2bc8` therefore models every reachable
wrapper instruction in 1 MiB RAM and the 14-field CPU state, while a
callback supplies the exact +0x0FAC call-entry effects. The model
applies the callee's `RETF 6` after a successful callback. A callback
refusal returns with the pending frame visible.

## Verification and gate

`tests/recovered/check_overlay_u38_link_message_2bc8.py` executes
the original 73 instruction bytes and intercepts only at +0x0FAC.
Thirty cases compare full 1 MiB RAM, all 14 CPU fields, and the
five-word callee-entry frame across null, both matching marker, and
three nonmatching marker branches, DS/SS alias, and stack wrap.
The focused C11 `-Wall -Wextra -Wpedantic -Werror` ASan/UBSan CTest
covers the same branches and callback abort.

Six DOSBox-X normal-core synthetic far-entry frames execute the
original wrapper with a deterministic +0x0FAC stub. All 14 CPU words,
52 stack bytes, and `DS:6818` match C. The COM, original-byte copy,
log, and capture stay under ignored `resources/u38-2bc8-parity/native/`.
Capture SHA-256:
`76cd00aebaf9467fcc36de9d0795b6718782ee13fd29c8e8ce0fc65bf193348d`.

The native callee is synthetic, and there is no reached game caller
frame. Full +0x0FAC effects and integration remain open. This is a
provisional wrapper candidate with no strict identity or count claim.
