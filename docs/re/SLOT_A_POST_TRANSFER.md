# Slot-A world-save post-transfer state step

The first missing in-memory step after `pr_slot_a_native_restore` is DOS
v1.3 `GAME.OVR` unit `0x6F:+0x18EA..+0x191D`. The new
`pr_slot_a_post_transfer_apply` composes with the adapter's ten transferred
spans: it derives three DS bytes from window 0 and clears one window-1
word in original instruction order. It is a **bounded slice** of the larger
world-save loader, not a full translation of that loader or a C-native
one-Up/key dispatch. No strict full-C or identity count is claimed.

Before decoding, `deja "Poolrad u6f post transfer 18EA 1919 DS49F6 49F5
49E2 window0 1FE 1F8 window1 67C"` found no exact session. Reused and
checked the merged [key-dispatch bridge audit](SLOT_A_KEY_DISPATCH_BRIDGE_AUDIT.md),
[save transfer map](SLOT_A_NATIVE_SAVE_ADAPTER.md),
[party loader model](SLOT_A_PARTY_INPUT.md), and
[native key call/return](SLOT_A_NATIVE_KEY_CALL_RETURN.md). pC independently
confirmed the exact span and next instruction boundary in inbox i658;
this C lane edits only uniquely named files.

## Byte boundary and effects

| Ignored input/span | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| slot-A `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| unit `0x6F:+0x18EA..+0x191D`, file `0x1F8D0..0x1F903` | **52 bytes, 14 contiguous 16-bit instructions**, no branch/call | `0e4be4f81f729cb89cadda3cecdc6bc16084a666cc4fbee1e1e4a61e23a9fed8` |

The last transfer call is at `+0x18DA`, followed by runtime close at
`+0x18E5`. The modeled step starts at `+0x18EA`. The final instruction
starts `+0x1919` and ends `+0x191D`. The next instruction at `+0x191E`
copies the loader's local character count to another local byte; the
party loop starts `+0x1926`.

| Unit offsets | Source width and effect |
| --- | --- |
| `+0x18EA..+0x18F5` | read **word** window-0 `+0x1FE`, logical `SHR AX,1`, store low **byte** at DS:`49F6` |
| `+0x18F8..+0x1904` | reload that **word**, `AND AX,1`, store low **byte** at DS:`49F5` |
| `+0x1907..+0x1910` | read window-0 **byte** `+0x1F8`, store DS **byte** `49E2` |
| `+0x1913..+0x191D` | zero AX, store **word** at window-1 `+0x67C` (two bytes) |

The three DS names remain address-based because their meanings are not
required by this body. The source word is read twice, as in DOS. This
matters if the far window changes between reads in a richer host; this
bounded adapter has no callback between them and uses one mutable frame.
Null arguments return before any effect; the DOS routine itself has no
null-pointer guard.

## Saved case and separate later effects

The hash-pinned save has window-0 word `+0x1FE=3`, byte `+0x1F8=4`, and
window-1 word `+0x67C=6`. The C step computes DS bytes
`[49F6:49F5:49E2]=(1,1,4)` and clears word `+0x67C` to zero, leaving
the rest of the transferred windows and party bytes unchanged. An
independent read of p9's ignored pre-key-call RAM snapshot
`pre-call-3395-ram.bin` (655,360 bytes, SHA-256
`4c145e29c105ceb2207b6015965af630ebeae91505f60b7606155e2939b92cdc`)
finds the same three DS bytes `1/1/4`. Its later window-1 counter is not
compared to this step's zero: the party insertion path increments the word
once per loaded member after `+0x1926`, giving six in the saved case.

The runtime close, local count copy, six character files, sidecars,
post-load callbacks, menu restored flag, and unit `0x1C:+0x32F8..+0x3386`
gameplay initialization still precede the actual key call at `+0x3395`.
The existing save adapter's `PR_SLOT_A_NATIVE_BLOCKED_KEY_DISPATCH`
result therefore remains appropriate. Feeding saved state straight into
unit `0x68` entry 2 would bypass those effects; this PR does not do that.

## Reproduction

`bash tests/replay/check_slot_a_post_transfer.sh` compiles under C11 with
`-Wall -Wextra -Wpedantic -Werror` and sweeps every 16-bit source word,
checking the 8-bit narrowing, parity bit, byte copy and two-byte clear.
Passing the ignored save path adds a private numeric C check. Passing the
ignored pre-call RAM path as a second argument also verifies whole-file
hashes, the exact 52-byte source span and live DS bytes. No game bytes,
strings, media or RAM dumps are checked in.
