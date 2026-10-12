# Saved slot-A load: live party-loop window word

This address/hash/numeric-only note samples one window-1 word during one
native DOS v1.3 slot-A **load**. It stops immediately after unit `0x6F` clears
the word and at that unit's character-record loop exit. The result supplies
one live state input for the saved-turn C bridge. It does not trace each
insertion, send an Up key, claim C-native parity, or change a routine count.

Before tracing, `deja "Poolrad u6F 19F7 party counter window1 67C slot-A
native replay"` found no indexed match. The checked-in
[post-transfer model](SLOT_A_POST_TRANSFER.md) supplied the clear-site lead,
[party-input audit](SLOT_A_PARTY_INPUT.md) supplied the loop-exit lead, and
[prior input trace](SLOT_A_NATIVE_KEY_CALL_RETURN.md) supplied the pinned
replay procedure. pC requested this single word in inbox i679. The live
pointer and values below were measured anew.

## Pinned ignored inputs and code identity

| Private input | SHA-256 |
| --- | --- |
| DOS v1.3 `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A `SAVGAMA.DAT` | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Private heavy DOSBox-X executable | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The original unit `0x6F` base is GAME.OVR file `0x1DFE6`. Its five-byte
instruction at `+1919..+191D`, file `0x1F8FF..0x1F903`, hashes
`00c93fc8679cc73ec12154bf8ae36c1e76376295f65e6b7bf2f01f5a67c98733`.
It stores zero to the window-1 word at far pointer `DS:[49D6]+067C`.
The next eight bytes at `+191E..+1925`, file `0x1F904..0x1F90B`, hash
`aa2def24716310124e24231da04b857567c30af9fc5808bbce41bb46ca9bc414`.
The 24-byte source span at loop exit `+19F7..+1A0E`, file
`0x1F9DD..0x1F9F4`, hashes
`1f6978734c06d54f5790a732739ec86e459d3d5027333186b85cbf0f355a8a35`.
All three spans match the loaded bytes at runtime segment `2A8E`, including
the 24-byte exit span in this run's RAM. Source decoding shows `+19F2`
branches to `+19F7` when the record-loop index reaches its bound; otherwise
`+19F4` jumps back to `+1937`.

## One-word live checkpoints

At the visible load-slot prompt, before selecting `a`, the heavy debugger
was paused and `BP 2A8E:191E` and `BP 2A8E:19F7` were armed. The source
identity above was checked against an earlier loaded RAM sample before the
run. No guest memory was edited.

| Stop | Guest state | Actual window pointer and target word | Private 640 KiB RAM SHA-256 |
| --- | --- | --- | --- |
| `2A8E:191E`, next instruction after the clear | `DS=14A0`, `ES=3A16`, `SS:SP=1D16:37B8` | `DS:[49D6]=3A16:0000`; word at `+067C` = **0** | `933a753805744effdda87934d528daeed13e450abf859e92889167de0d92a337` |
| `2A8E:19F7`, record-loop exit | `DS=14A0`, `SS:SP=1D16:37B8`, `AX=0006` | `DS:[49D6]=3A16:0000`; word at `+067C` = **6** | `72e239855b41424c3bc3ea927132a5002df42389a201954e5fd3c38f1b71a9a8` |

An earlier private pre-key-call RAM sample at unit `0x1C:+3395` (SHA-256
`4c145e29c105ceb2207b6015965af630ebeae91505f60b7606155e2939b92cdc`)
also has `DS:[49D6]=3A16:0000` and word `+067C=6`. That later sample alone
could not establish the value at the record-loop exit; the two stops above
do. The endpoint change `0→6` is consistent with the existing bounded
party model's saved-case aggregate of six. This run does not identify the
individual writes or execute that C model end to end.

## Reproduction and private-data boundary

In an isolated checkout with the pinned ignored inputs, prepare one private
run and launch the heavy debugger from a PTY:

```sh
tools/oracle/prepare_run.sh slot-a-party-67c-live-015
export PATH="$PWD/resources/dosbox-x-heavy-src/src:$PATH"
xvfb-run -a -s "-screen 0 1280x1024x24" \
  tools/oracle/launch.sh resources/oracle-runs/slot-a-party-67c-live-015 \
  -time-limit 1200
```

Wait for the visible code-word prompt, send Enter, select `l`, and pause at
the visible slot-A load prompt before selecting `a`. Arm the two breakpoints
above, resume, and select `a`. At each stop, use
`EV CS IP DS ES DI SS SP BP AX` and `MEMDUMPBIN 0000 0000 A0000`. Read
the little-endian far pointer at `DS:[49D6]` from each private dump, then
read the little-endian word at that pointer plus `067C`.

The original media, save, screenshots, debugger output, and RAM dumps stay
under ignored `resources/`. This public note contains only addresses,
hashes, numeric state, and instruction behavior.
