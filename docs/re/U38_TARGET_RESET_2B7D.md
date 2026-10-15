# Unit 0x38 public entry 103: target reset at +0x2B7D

## Provenance and boundary

PM assigned the next disjoint U38 leaf after the +0x2B20 branch. Entry
`+0x2B63` was already owned by the merged effects source, so this candidate
uses entry 103. The bounded `deja "poolrad U38 +2B7D +2BC8 entry103
entry104 strict C aliases callers"` query timed out after 25 seconds
without a session result. This work reuses the measured
[+0x101D target selector](U38_TARGET_SELECT_101D.md), including its
loaded resident far-call fixup, and the
[resident selector evidence](RESIDENT_CHARACTER_TARGET_SELECT.md). The
entry's own span, bytes, and effects were checked independently.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

The complete public entry is U38 `[+0x2B7D,+0x2BC8)`, `GAME.OVR`
file `[0x117D8,0x11823)`: 75 contiguous bytes, 25 instructions,
SHA-256 `8cdf5fe20286cd39394d2b2e5ed4dffa01a318fb10a62b35548bb19ff9a0d323`.
The public entry table names only index 103 at `+0x2B7D`; entry 104
starts at `+0x2BC8`. No loader fixup lies in this wrapper. Its one
call is the decoded near call at `+0x2B8C` to `+0x101D`; no decoded
near call from another public U38 entry targets `+0x2B7D`.

## Original behavior and C model

The wrapper passes the active character pointer from `DS:5D92/5D94`
to `+0x101D`. A nonnull selected target whose byte `+0x32` is nonzero
leaves `DS:6818` untouched. Otherwise, it clears `DS:6818` if the
active character's signed byte `+0x2E` is greater than zero, or if
signed byte `+0x73` is less than four. It preserves the byte in the
remaining cases. The return is `RETF 10`; five caller words are
discarded without inspection.

`pr_u38_target_reset_2b7d` composes the merged +0x101D C model in
1 MiB RAM and a 14-field CPU state. It writes the original near-call
return frame and locals, reproduces `SUB`, `OR`, and signed `CMP`
FLAGS, and applies the exact far return. A selector callback refusal
leaves the pending far-call frame visible and reports an abort.

## Verification and gate

`tests/recovered/check_overlay_u38_target_reset_2b7d.py` executes the
original complete +0x2B7D wrapper and +0x101D helper bytes. Across
288 cases it compares the entire 1 MiB RAM, all 14 CPU fields, and
each six-word resident callback frame. Cases vary selector result,
all reset branches, loaded far-call segment, DS/SS alias, and stack
wrap. The focused C11 `-Wall -Wextra -Wpedantic -Werror` ASan/UBSan
CTest covers the branches and callback abort.

Six DOSBox-X normal-core synthetic far-entry frames execute the
original wrapper and helper with a deterministic resident stub.
All 14 returned CPU words, 52 stack bytes, and `DS:6818` match C.
`tests/recovered/native_overlay_u38_target_reset_2b7d.py` keeps the
COM, original-byte copies, log, and capture under ignored
`resources/u38-2b7d-parity/native/`. Capture SHA-256:
`a67e322bed2151a16ff661099224a69f21b9cc0c4f0b49d16281a189afb0cb62`.

These native frames are synthetic and relocate the resident target.
There is no reached game caller frame for this entry. The +0x101D
selector's game-versus-library ownership is also a PM integration
gate. This is a provisional full-C candidate with no strict identity
or count claim.
