# Unit 0x38 public entry 35: target dispatch at +0x1086

## Provenance and extent

The bounded `deja "poolrad U38 +1086 entry35 +0FAC +101D full C parity"`
query timed out without a prior-session result. This work reuses the checked
[+0x101D target selector](U38_TARGET_SELECT_101D.md), its
[resident selector](RESIDENT_CHARACTER_TARGET_SELECT.md), and the existing
[+0x0FAC callback boundary](U38_LINK_MESSAGE_2BC8.md). Entry bytes and
boundaries were independently checked against ignored local inputs:

| Input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

Public entry 35 is the complete 59-byte unit `0x38:[+0x1086,+0x10C1)`,
`GAME.OVR` file `[0xFCE1,0xFD1C)`, SHA-256
`57814ae9517b0228e944dfed658634327392f6cc64275741c78facd62b103c5e`.
It has 24 contiguous instructions, no wrapper fixups, near calls to
`+0x101D` at `+0x1092` and `+0x10A2`, and a conditional near call to
`+0x0FAC` at `+0x10BA`. It ends with `RETF 10`.

## Behavior and dependencies

The entry calls +0x101D with the active character pointer at
`DS:5D92/5D94`. If the returned far pointer is null, it returns. For a
nonnull first result, it calls +0x101D again and reads byte `+0x32` from
the second returned far pointer. A nonzero byte returns. A zero byte
passes caller words `[BP+0x0E]` and `[BP+0x0C]`, plus a word whose low
byte is `0x64`, to +0x0FAC. The high byte retains the high byte of the
second target offset. The callee returns with `RETF 6`; this entry then
returns with `RETF 10`.

`pr_u38_target_dispatch_1086` models every instruction in 1 MiB RAM and
the 14-field CPU state. It composes the merged +0x101D model for both
near calls. That helper's resident far call to loaded segment `:2E8B`
remains a callback boundary. The distinct 113-byte +0x0FAC callee is
also a callback boundary; its full effects and its four loaded far-call
fixups are not modeled here. A callback refusal preserves the respective
pending call frame and returns a separate status.

## Measured parity and limit

The original-byte Unicorn checker executes the complete +0x1086 body
and both actual +0x101D calls, intercepting only resident `:2E8B` and
+0x0FAC. Across 192 cases it compares the complete 1 MiB RAM, all
14 CPU fields, and each callback entry frame. Cases cover all eight
selector modes, zero and nonzero target markers, two loaded resident
segments, and three DS/SS/stack layouts.

Four DOSBox-X normal-core synthetic far-entry frames execute the pinned
entry and +0x101D helper with deterministic resident and +0x0FAC stubs.
They compare all 14 CPU words, 52 stack bytes, and `DS:6818`. The native
capture stays under ignored `resources/u38-1086-parity/native/`, SHA-256
`ad2d4c770870e6437aa4fd27efedefb4d7a08205dc048ad266f36d32b541c15f`.
Focused C11 `-Wall -Wextra -Wpedantic -Werror` ASan/UBSan CTest covers
null target, both marker branches, selector aborts at each call, and
+0x0FAC callback refusal.

These are bounded behavior checks with synthetic dependency stubs. A
natural game caller frame has not been reached, so game identity and
strict function count remain provisional.
