# Saved slot-A load: live restored flag and first gameplay gate

This address/hash/numeric-only note covers one native DOS v1.3 slot-A **load**
from the pinned save through the first gameplay gate at unit `0x1C:+334D`.
No Up key was sent in this run. It establishes the restored-flag value and the
branch taken before the first gameplay input call. It adds no C routine, game
routine count, or C-native parity claim.

Before the replay, `deja "poolrad DS 49F8 unit 1c 334D 3352 ECL block load
slot A last writer"` found no indexed match. The checked-in
[gameplay path](GAMEPLAY_PATH.md) supplied the `+334D` gate lead, and the
[prior native input trace](SLOT_A_NATIVE_KEY_CALL_RETURN.md) supplied the
hash-pinned load procedure. A peer's inbox correction identified unit `0x6C`
entry 1 `+040B..+0417` as the menu setter path. The locations and values
below were checked in this run.

## Pinned inputs and code identity

| Ignored input | SHA-256 |
| --- | --- |
| DOS v1.3 `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A `SAVGAMA.DAT` | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Private heavy DOSBox-X executable | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The unique menu setter signature is at original GAME.OVR file offset
`0x19D91`. The 20-byte source span `0x19D8A..0x19D9D`, corresponding to
unit `0x6C:+040B..+041E`, hashes
`8730802942a7db8bd87d752ea3434402585576bf6b1c65c42f35b5a85e2dcf32`.
The same 20 loaded bytes at `2648:040B..041E` match exactly, with the same
hash. The decoded branch compares `DS:[49F3]` with zero at `+040B`, jumps
over the store when zero, and otherwise stores `1` to `DS:[49F8]` at
`+0412`; `+0417` is the following jump.

The original 37-byte unit `0x1C:+334D..+3371` gate span at GAME.OVR file
`0x4093..0x40B7` hashes
`6bb4c4c2454fb0cc05e74871abe02335b1e5b176ec8b780ab104b166fcbbe68c`.
The loaded span at `3582:334D..3371` hashes
`6076b7276aae55951368b0ff35cdee762b40ca41f17beaf1dd7f81a858c5a18b`;
only the relocated far-call operand bytes at `+335B/+335C` and
`+3360/+3361` differ. The first ten loaded bytes, covering the compare and
conditional branch, match the source and hash
`32d08c82ce749e6fc2d3777e3a714f810e29a84920fff1d6fa6efaa303b989e1`.
The gate compares `DS:[49F8]` with zero at `+334D`; `+3352` branches to
`+335D` when nonzero, bypassing the conditional ECL block-load call at
`+3358`.

## Live load and branch observations

At the visible `LOAD WHICH GAME` prompt, before selecting slot A, the guest
was paused at `2116:0492` with `DS=14A0`. `BP 3582:334D` and
`BPM 14A0:49F8` were armed. The memory watch reports value **changes** at
instruction boundaries; it does not detect a write that stores the same
value. A RAM dump established its initial byte value as zero.

| Stop | Registers and measured DS bytes | RAM SHA-256 |
| --- | --- | --- |
| Load prompt, before `a` | `DS:49F3=0`, `DS:49F8=0` | `1032df3ca686cbd17271b7dbdd8893f431b31ac71abaef09afabb4586dfa7a28` |
| `BPM` after `2648:0412` store | watch reports `49F8: 0→1`; `CS:IP=2648:0417`, `DS=14A0`, `SS:SP=1D16:3DD0`, `DS:49F3=2` | `5ebf6b3716e36eca0c2527da85cad36f52fea1884e87ad2ec59f888770b287f5` |
| Before `3582:334D` compare | `DS=14A0`, `SS:SP=1D16:3EF6`, `AX=A014`, `DS:49F3=4`, `DS:49F8=1` | `30373cebcf9d0edcaf6e4fe23aa5348710251cc08e2aa726b0e6243f93e8bf52` |
| `3582:335D` branch target | `DS=14A0`, `SS:SP=1D16:3EF6`, `AX=A014` | no additional dump |

No further `49F8` value-change watch fired between the `+0412` store and
the `+334D` gate. The guest then stopped at `+335D` with the same stack
pointer and AX, confirming that this restored path skipped the `+3358`
call. This watch does not identify any same-value writes in that interval,
and it does not establish a later one-Up route.

## Reproduction and private-data boundary

In an isolated checkout with the pinned ignored DOS inputs, prepare and
launch one private run with the heavy debugger attached to a PTY:

```sh
tools/oracle/prepare_run.sh slot-a-49f8-gate-live-014
export PATH="$PWD/resources/dosbox-x-heavy-src/src:$PATH"
xvfb-run -a -s "-screen 0 1280x1024x24" \
  tools/oracle/launch.sh resources/oracle-runs/slot-a-49f8-gate-live-014 \
  -time-limit 1200
```

Wait for the visible code-word prompt, send Enter, select `l` at the menu,
then pause at the visible load-slot prompt before selecting `a`. In the
debugger, arm `BP 3582:334D` and `BPM 14A0:49F8`; use
`MEMDUMPBIN 0000 0000 A0000` for private RAM samples and
`EV CS IP DS SS SP BP AX` for stop registers. Resume, select `a`, and
capture the byte-change and gate stops. At the gate, arm `BP 3582:335D`
and resume once to confirm the branch destination.

The original media, save, screenshots, debugger stream, and RAM dumps remain
under ignored `resources/`; this note contains only addresses, hashes,
numeric state, and instruction behavior.
