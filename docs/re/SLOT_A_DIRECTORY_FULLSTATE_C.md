# Saved slot-A directory boundary: CPU and RAM replay

This adds a separate bounded C state path from the reached SYSTEM
`131C:1821` DOS directory service through the unit `0x6F:+02DA`
IOResult return. It leaves the existing `pr_u6f_postinsert_02ae` callback
model intact. The preceding pathname conversion and the later branch or
retry in unit `0x6F` are outside this boundary. No full routine count or
end-to-end one-Up parity is claimed.

Before implementation, `deja "poolrad INT21 39 directory wrapper IOResult
CPU RAM journal"` found no indexed match. The merged
[directory effect note](SLOT_A_NATIVE_DIRECTORY_INT21_EFFECT.md) supplied
the independently checked `+1821`, `+1823`, `+1826`, `+1829`, `+02D5`,
`+04A2`, `+04A8`, and `+02DA` stops. The new run below directly captures
the complete CPU tuple at each one, including the previously untranscribed
`+1829` state.

## Pinned inputs and code

Fresh ignored run `resources/oracle-runs/slot-a-native-directory-fullstate-044`
was prepared with `tools/oracle/prepare_run.sh`. All files under that run,
including save, debugger stream, screenshots, and RAM images, remain ignored.

| Input | Bytes | SHA-256 |
| --- | ---: | --- |
| START.EXE | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A SAVGAMA.DAT | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Heavy DOSBox-X | 47,259,176 | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

Original START SYSTEM segment `0AF8` and loaded segment `131C` have
identical bytes in each span below. The full wrapper span extends two bytes
beyond the earlier note's 19-byte prefix, including the `RETF 4` operand.

| Span | Bytes / instructions | Original and loaded SHA-256 |
| --- | ---: | --- |
| SYSTEM directory wrapper `+17D6..+17EA` | 21 / 9 | `b221817559d0f699163e7a32cb1424df5808855ac7dcf3becdc86e85d6f7803e` |
| SYSTEM DOS helper `+181B..+1829` | 15 / 9 | `978fabd0bca91df715152943c57b87c26e835dbd9533fbb60843e32bfc86908b` |
| SYSTEM IOResult `+04A2..+04A8` | 7 / 3 | `d70153d7282a717eca08c439be05fb911569333751907973b73ca0b475e08d1b` |
| Loaded unit `0x6F:+02D0` far call | 5 / 1 | `730af1089e243d4070cd90da55bec6daefbae79c8f6215e6e9b303b41a882a4f` |
| Loaded unit `0x6F:+02D5` far call | 5 / 1 | `8b8f759c2e82066bcf810b2e283011da603f25ca0d1bc9af8bbc5d4747ee40e9` |

## Reached native states

For all rows, the remaining modeled fields are constant:
BX=`35A0`, CX=`0000`, DX=`3526`, SI=`36BB`, DI=`3531`,
ES=`1D16`, FS=`0000`, GS=`0000`, SS=`1D16`.
The table plus these fields is the complete 16-field C CPU state at each
direct debugger stop. RAM hashes cover linear `0x00000..0x9FFFF`.

| Stop | CS:IP | DS | SP | BP | AX | FLAGS | 640 KiB RAM SHA-256 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| pre-INT | `131C:1821` | `1D16` | `3522` | `35A6` | `3900` | `7246` | `d759a7a8785ae9da0e4b7e5ba6e2da9094d0bffe9afb217d6016960bc06f6a17` |
| post-INT | `131C:1823` | `1D16` | `3522` | `35A6` | `0005` | `7247` | `9f22433503ac85824a6fbb8bf1a3db57298f2453193e1c31f42e491db2885f0b` |
| pre-store | `131C:1826` | `14A0` | `3524` | `35A6` | `0005` | `7247` | same as post-INT |
| post-store | `131C:1829` | `14A0` | `3524` | `35A6` | `0005` | `7247` | `68918d9e13e3e8f2913ee5fd41ebbe3f12c4f1ca36b08d1509bd44bbb12a8ef2` |
| wrapper return | `2A8E:02D5` | `14A0` | `35B0` | `37B2` | `0005` | `7247` | same as post-store |
| IOResult entry | `131C:04A2` | `14A0` | `35AC` | `37B2` | `0005` | `7247` | `fecaa4875970fdf808e8a9cc8ff0daf1d3c34b10bfb8d1ca00a8f919b0043e98` |
| after exchange | `131C:04A8` | `14A0` | `35AC` | `37B2` | `0005` | `7246` | `9f2213742ba9c8bd870b3109c703c9cd58e716d0a5e4df89d9496818f0ad3c55` |
| caller return | `2A8E:02DA` | `14A0` | `35B0` | `37B2` | `0005` | `7246` | same as after exchange |

Across the fresh INT service, four net RAM bytes changed in two runs,
`0x0816E..0x0816F` and `0x20680..0x20681`. The earlier run in the linked
note had 23 net bytes in three runs; DOS scratch state and the initial low
memory values differed. The reached error-word store changed only linear
`0x18E26`, `0→5`; IOResult changed it `5→0`. The far call changed four
return-frame bytes at `0x2070C..0x2070F`. AX=5 is an observed result for
this replay, not a constant in the C model.

## C boundary and verification

`pr_slot_a_directory_step` starts at the already reached `131C:1821`.
With no service callback it returns `BLOCKED_DOS_SERVICE`, preserving that
CPU/RAM state. A supplied callback receives the complete pre-INT CPU and
read-only RAM, and returns a post-INT CPU plus an ordered byte-write journal.
The reducer validates the entire journal, including successive old values
for repeated addresses, before applying any write. It requires the reached
post-INT `131C:1823` PC. The callback owns service AX, carry, other CPU
effects, and RAM effects. The C stepper then executes the reached POP DS,
conditional error-word store, return-frame unwind, far call, XOR/XCHG, and
far return. Only the saved loaded segments and return targets are accepted.

The source-only optional verifier
`tests/recovered/check_slot_a_directory_fullstate_native.c` reads the
ignored RAM captures. It uses the sorted **net** pre/post-INT byte delta as
one valid callback journal, then compares all 16 modeled CPU fields and all
655,360 captured RAM bytes after each C step. That order is a projection
from snapshots, not an observation of DOS's internal write order. On run
044, all eight comparisons passed. The uncaptured RAM above `0x9FFFF`,
the pathname conversion before `+1821`, and later unit `0x6F` control flow
remain outside this parity claim.

Reproduce with pinned ignored inputs: prepare a fresh run; launch the
hash-pinned heavy DOSBox-X using `tools/oracle/launch.sh` under Xvfb; pass
the codeword prompt with Return; select `l`; at the slot prompt pause with
Alt+Pause, set `BP 2A8E:02D0`, `RUN`, then select `a`. At `+02D0`, arm
`BP 131C:1821`, `BP 131C:1823`, `BP 131C:1826`, `BP 131C:1829`,
`BP 2A8E:02D5`, `BP 131C:04A2`, `BP 131C:04A8`, and
`BP 2A8E:02DA`. At each stop use
`EV CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS` and
`MEMDUMPBIN 0000 0000 A0000`; move `MEMDUMP.BIN` to the matching ignored
`ram-<stop>.bin` before continuing. Then run:

```sh
run=resources/oracle-runs/slot-a-native-directory-fullstate-044
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude \
  src/recovered/slot_a_directory_fullstate.c \
  tests/recovered/check_slot_a_directory_fullstate_native.c \
  -o "$run/check_native"
"$run/check_native" "$run"
```

The required public synthetic test covers successful service completion,
error 5, a different error, missing service, repeated writes, invalid
journal values, and the wrapper/IOResult stack and flag transitions.
