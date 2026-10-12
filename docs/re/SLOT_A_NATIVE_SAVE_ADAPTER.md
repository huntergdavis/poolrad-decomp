# Slot-A C-native save adapter boundary

This adapter is the first isolated C-native state load for the one-Up parity
case in issue #3. It restores the ten world-save transfer spans and reports
the first executable blocker. It does not execute an Up key, load matching
character records or map blocks, run post-load effects, or establish C parity.

Before implementation, `deja "poolrad C native slot A save state adapter pr_dos_savgam_open read_core block one Up harness"` returned no direct prior match. `deja "DOS SAVGAM native replay"` surfaced the earlier save-frame and native reachability sessions. This work reuses the merged [save-frame map](../formats/DOS_SAVGAM.md), [gameplay loop trace](GAMEPLAY_PATH.md), [Up preflight boundary](U68_UP_PREFLIGHT_068D.md), and [native DOS observations](SLOT_A_ONE_UP_NATIVE_COORDINATE_WATCH.md); the adapter and its test check the data transfer afresh.

## Inputs and exact transfer boundary

| Ignored input | Size | SHA-256 |
| --- | ---: | --- |
| Slot-A world save | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

Unit `0x6f` entry 8, `+0x17D5..+0x18DA`, reads ten spans. The adapter calls
`pr_dos_savgam_open`, `pr_dos_savgam_read_core`, and
`pr_dos_savgam_block` for each span, checks every width, then deep-copies
the bytes into uniquely named replay state. File offsets and widths are:

| Transfer | File start | Width | Native destination |
| ---: | ---: | ---: | --- |
| 0 | `0x0000` | 1 | initial ECL byte |
| 1 | `0x0001` | `0x800` | window 0 |
| 2 | `0x0801` | `0x800` | window 1 |
| 3 | `0x1001` | `0x400` | window 2 |
| 4 | `0x1401` | `0x1E00` | script buffer |
| 5 | `0x3201` | 5 | party bytes |
| 6 | `0x3206` | 1 | selector |
| 7 | `0x3207` | 1 | mode |
| 8 | `0x3208` | 1 | character count |
| 9 | `0x3209` | `0x148` | name slots |

The adapter also exposes the separately documented post-load ECL byte at
window 1 `+0x624` and map ID at window 0 `+0x1E4`. Exposing a value does
not execute the corresponding DOS fixup or its dependent calls. The source
parser validates length and layout, not the authenticity of every field in
an arbitrary same-size file.

## Numeric result and first blocker

The hash-pinned private smoke restored `(x,y,facing)=(15,4,6)` and window-0
words `(+0x18E,+0x190,+0x192)=(0,5,10)`. The adapter's one-Up attempt
returns `PR_SLOT_A_NATIVE_BLOCKED_KEY_DISPATCH` and leaves its state
unchanged. This is an explicit missing effect, not a simulated move.

The first missing executable function is the input route beginning at unit
`0x1c:+0x3395` inside gameplay entry `+0x32F8`, which calls unit `0x68` entry 2
`+0x0989`. The merged trace says entry 2 obtains a key and extended flag
through its prompt call, handles extended Up `0x48` by calling the translated
preflight `+0x068D`, and eventually returns a key byte in `AL`. Its complete
input/prompt call ABI, local key/flag frame, loop effects, and return
contract have not yet been translated into the C-native route. Therefore
the adapter stops before that call. Later script-gate, coordinate-writer,
and clock-call effects are not reached by this C program.

The DOS oracle's one-Up target is `(14,4,6)` with words `(1,5,10)`, live
direction bytes `(255,0)`, and window-1 `+0x594` bit 0 clear. Those are
reference observations only; this adapter claims none of those post-Up
results. It adds no game-routine count.

## Reproduction

Run `tests/replay/check_slot_a_native_save.sh` for the strict C11 synthetic
test. It compiles with `-Wall -Wextra -Wpedantic -Werror`, checks all ten
transfer boundaries, distinct little-endian words, deep-copy behavior,
failure immutability, and the explicit blocker. The same script accepts one
hash-pinned ignored world save under this clone's `resources/` to run the
private numeric smoke; the path and bytes stay outside Git. The private run
reported the initial tuple above and the key-dispatch blocker.
