# Unit 0x38 public entry 101: linked-target roll at +0x2B20

## Source and prior work

PM queued this disjoint follow-on after the provisional U38 +0x2AE6
entry. The bounded `deja "poolrad U38 +2B20 entry101 original-byte far
call parity"` query returned no session result within 25 seconds. The
callee mapping and model reuse the merged
[U9E entry-9 roll latch](U9E_ENTRY9_ROLL_LATCH_0E2E.md) and its
[entry-8 dice evidence](U9E_ENTRY8_ROLL.md), checked here against the
original call fixup and bytes.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

The complete U38 public entry 101 is `+0x2B20..+0x2B63`, file
`[0x1177B,0x117BE)`: 67 original bytes / 25 aligned instructions,
SHA-256
`9a322acac07936548b9d1257854e48fc256e29bf2dec0f4c58dfab9ca639366b`.
The next public entry starts at `+0x2B63`. The sole call is the
fixup-confirmed `LCALL B0:004D` at `+0x2B52`, with loader segment
operand at `CS:2B55`. `B0:004D` is the U9E entry-9 roll-latch stub;
the original `B0` operand is not the loaded segment.

## Exact behavior and C boundary

The entry reads the active character from `DS:5D92/5D94`, then the
character's linked far pointer at `+CC/+CE`. A null link returns
without a call. A nonnull link is checked for byte `0x55` at `+2E`.
Other values also return without a call. On a match, it pushes two
words derived from the `AX` value formed by ORing the link offset and
segment: low bytes 1 (count) and 6 (range), with the original AH
retained in both words. The far call to U9E entry 9 rolls and stores
the count latch at `DS:681C`. U38 then clears AH, increments AX, and
stores AL at `DS:6818`. It exits with `RETF 10`, removing five caller
words that it never reads.

`pr_u38_link_roll_2b20` models all 67 original bytes in 1 MiB RAM and
the 14-field CPU. It writes the far-call return frame, reads the live
loaded segment operand, preserves callback RAM/CPU effects, and models
`SUB`, `OR`, `CMP`, `XOR`, `INC`, and `RETF 10` FLAGS and stack effects.
The explicit callback enters at loaded segment `:004D` with return IP
`2B57`, return CS, range word, and count word. It executes the complete
callee including `RETF 4` and must return at caller `CS:2B57`. The
focused adapter composes merged `pr_u9e_entry9_roll_latch_0e2e` and
supplies its still-explicit entry-8 roll callback.

## Verification and limits

The 54-case Unicorn checker executes original U38 +0x2B20, a synthetic
loader-stub jump, and original U9E entry 9 +0x0E2E. It intercepts
only the deeper U9E entry-8 roll callback and compares the full 1 MiB
RAM, all 14 CPU fields, the four-word U38 far-call frame, and the
four-word U9E near-call frame against the composed C models. Cases
cover null/wrong-kind exits, real one-die calculation and overridden
AL=0x0F/0xFF results, two loaded fixup segments, DS/SS alias, and
16-bit stack wrap. Focused strict C11 `-Wall -Wextra -Wpedantic -Werror`
ASan/UBSan CTests pass for U38 and merged U9E entry 9, including
callback abort and the increment's AF and byte wrap.

Five DOSBox-X normal-core synthetic frames execute an actual far
entry into the original U38 body and its actual far call through the
synthetic loader stub to the original U9E entry-9 body. The deeper
entry-8 roll uses a deterministic synthetic stub. All 14 returned CPU
words, 52 stack bytes, and `DS:6818/681C` match the C composition.
Capture SHA-256:
`b65d3dbb417e9af87251c4c613263a4971ce2eaba652489db34cc7d04b3c2fe0`.
Generated COM, copied body, logs, and capture stay under ignored
`resources/u38-2b20-parity/native/`.

The native frames are synthetic; there is no reached gameplay frame
for this entry. The merged high-level U9E entry-8 dice API does not
expose its complete CPU side effects, so those remain the callback's
obligation. This is a provisional game-role candidate for PM review;
no identity or strict count credit is claimed here.
