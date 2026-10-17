# Unit 0x38 public entry 80: effect clear at +0x264B

## Provenance and identity gate

The bounded `deja "poolrad U38 +264B entry80 +101D effect clear strict C parity"`
query timed out without a prior-session result. This work reuses the checked
[+0x101D target selector](U38_TARGET_SELECT_101D.md), its
[resident selector](RESIDENT_CHARACTER_TARGET_SELECT.md), and the
[effect-table installer](U38_EFFECTS_INSTALL_31A2.md). Source bytes and extent
were independently rechecked against these ignored local inputs:

| Input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

The complete public entry is unit `0x38:[+0x264B,+0x268A)`, `GAME.OVR`
file `[0x112A6,0x112E5)`: 63 bytes / 22 contiguous instructions, SHA-256
`2350f9f299e129564727b0861075847546ca9ca91d1ae6e71f7c17e45209a6fb`.
The next public entry begins at `+0x268A`. There are no wrapper loader fixups.
Its sole call at `+0x265A` is a near call to `+0x101D`, which has a
fixup-backed far call to the resident selector at `:2E8B`.

The +0x31A2 installer writes the entry-80 stub pointer `0041:01B0` into
`DS:69A8` (effect-table item 96), with a loader segment fixup at +0x366F.
The U9E dispatcher calls through `DS:6828`-based table pointers. This is a
game-effect route, but no native frame from item 96 into this entry has been
reached. The exact game caller and function identity remain provisional.

## Behavior and model boundary

The entry passes the active character pointer from `DS:5D92/5D94` to
`+0x101D`. A null returned far pointer clears `DS:6818`. With a nonnull
target, it preserves `DS:6818` if target byte `+0x32` is nonzero, or if
byte `+0x32` is zero and byte `+0x31` is `0xB1`. Otherwise it clears
`DS:6818`. It exits through `RETF 10` after five caller words.

`pr_u38_effect_clear_264b` models the complete wrapper using a 1 MiB RAM
image and 14 CPU fields. It composes the merged +0x101D C model and exposes
its loaded-segment resident call at `:2E8B` through a callback. The callback
owns the resident register, flag, and memory effects; refusal leaves the
callee-entry frame visible and returns `PR_U38_264B_CALL_ABORT`.

## Verification and limits

The original-byte Unicorn checker executes all 63 wrapper bytes and all
105 bytes of +0x101D, including the actual near call and relocated far call.
Across 48 cases it compares the entire 1 MiB RAM, all 14 CPU fields, and
each resident callback frame. Cases vary null/selected/fallback outcomes,
target bytes, amount, loaded far-call segment, and DS/SS layouts.

Four DOSBox-X normal-core synthetic far-entry cases execute the original
wrapper and helper. They compare all 14 CPU words, 52 stack bytes, and the
resulting `DS:6818` byte against the C model. The pinned native capture is
under ignored `resources/u38-264b-parity/native/`, with result SHA-256
`9361f8787dcb555db1d64ca012f88a27b47d5fb794d8f750c7320b623d551434`.
The resident target is relocated to a deterministic synthetic stub. This is
bounded behavior parity, not a reached game caller. Focused strict C11
`-Wall -Wextra -Wpedantic -Werror` and ASan/UBSan tests cover the same paths
and a refused callback.

No strict identity or function-count credit is claimed. PM requires an
actual item-96 dispatcher reach for that gate, and future SSH pushes require
PM exact-head approval. This branch remains local pending that review.
