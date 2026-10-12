# Saved slot-A unit 0x6F decision after IOResult

This bounded continuation starts at loaded `2A8E:02DA`, immediately after
the merged [full-state directory boundary](SLOT_A_DIRECTORY_FULLSTATE_C.md).
It executes five instructions through the observed first branch decision and
stops at `2A8E:031F` for the saved result 5. Synthetic result 0 also reaches
`+031F`; another result stops at `+02E9` before message setup and the later
far call. The instructions at `+031F` and later retry/return behavior are
outside this C path. No enclosing routine, DOS service, or one-Up parity is
claimed.

Before implementation, `deja "poolrad saved slot A unit 6F 02DA IOResult
branch retry next external call"` found no indexed match. The merged
[post-insert note](SLOT_A_NATIVE_U6F_POST_U9B_DIRECTORY.md) identified the
saved result 5 and broad unit bounds; the full-state directory note supplied
the reached `+02DA` CPU/RAM handoff. Run 045 below independently checked the
next five instructions and all intermediate states.

## Pinned inputs and instruction proof

Fresh ignored run `resources/oracle-runs/slot-a-native-u6f-02da-045` uses:

| Input | Bytes | SHA-256 |
| --- | ---: | --- |
| START.EXE | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A SAVGAMA.DAT | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Heavy DOSBox-X | 47,259,176 | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

Original `GAME.OVR[0x1E2C0..0x1E2CE]` and loaded
`2A8E:02DA..02E8` are identical: 15 bytes, SHA-256
`8546b6820e0d641281e64f2f3bfd35e1b2595dbf6590bbe50fce50999f94b6cc`.
An independent 8086 disassembly has five instructions: `+02DA` stores AX
at SS:[BP−2], `+02DD` compares that word with zero, `+02E1` branches on
equality to `+031F`, `+02E3` compares the word with 5, and `+02E7`
branches on equality to `+031F`. Nonmatching values fall through to
`+02E9`. These are the entire translated span.

## Native states and RAM effect

At every stop CS=`2A8E`, DS=`14A0`, ES=`1D16`, FS=GS=`0000`,
SS=`1D16`, SP=`35B0`, BP=`37B2`, AX=`0005`, BX=`35A0`, CX=`0000`,
DX=`3526`, SI=`36BB`, and DI=`3531`. Those constants plus each row's
IP/FLAGS form the complete 16-field C CPU state. Each RAM hash covers
linear `0x00000..0x9FFFF` (655,360 bytes).

| Stop IP | FLAGS | 640 KiB RAM SHA-256 |
| --- | --- | --- |
| `02DA` | `7246` | `d05397fca1b0a5275bf3043a7b8a6602bf4734204f8f60e49f9085690e247737` |
| `02DD` | `7246` | `85d06b2407f87dbad815dd70d3b3d5efe9950d5eb14e66fa1a0152a8f68187f6` |
| `02E1` | `7206` | same as `02DD` |
| `02E3` | `7206` | same as `02DD` |
| `02E7` | `7246` | same as `02DD` |
| `031F` | `7246` | same as `02DD` |

The only RAM changes from `+02DA` to each later stop are linear
`0x20910..0x20911`, the SS:[BP−2] word: `0x19E6` to `0x0005`.
The native first JE at `+02E1` is not taken (ZF=0); the second at
`+02E7` is taken (ZF=1). The ignored normalized EV transcript at
`resources/oracle-runs/slot-a-native-u6f-02da-045/ev-transcript.txt`
has SHA-256
`a7141f466256f2f139517982997c2afd5d774f6b07e5dfe0d548b1eba1e1bb65`.
It records the six direct EV observations in command column order; the
optional verifier reads it rather than embedding the expected CPU states.

## C boundary and reproduction

`pr_slot_a_u6f_02da_step` takes the existing directory full-state CPU/RAM
type. One call executes one instruction at the five accepted IPs. The store
and comparisons address each word byte with a separately wrapped 16-bit
segment offset and a 20-bit physical address. CMP computes the six 8086
arithmetic flags and preserves all others. Any unavailable word byte,
unexpected CS/IP, or terminal stop returns `BOUNDARY` without mutation.
The synthetic test covers saved result 5, zero, another error, changing
the memory word between instructions, flags, offset wrap, and boundary
preservation. It does not substitute for the native comparison.

To reproduce the native capture, prepare a fresh ignored slot-A run with
`tools/oracle/prepare_run.sh` and launch the pinned heavy DOSBox-X under
Xvfb using `tools/oracle/launch.sh`. Dismiss the codeword prompt, choose
Load Saved Game, pause at the slot prompt, arm `BP 2A8E:02D0`, resume,
and select slot A. At `+02D0`, arm `BP 2A8E:02DA`, `BP 2A8E:02DD`,
`BP 2A8E:02E1`, `BP 2A8E:02E3`, `BP 2A8E:02E7`, and
`BP 2A8E:031F`. At each stop run
`EV CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS` and
`MEMDUMPBIN 0000 0000 A0000`; move each binary dump to ignored
`ram-<stop>.bin` before continuing. Normalize the six EV rows in the
stated column order into ignored `ev-transcript.txt`.

For the pinned run, verify the transcript hash and compare all 16 CPU
fields and all 655,360 RAM bytes after each modeled instruction:

```sh
run=resources/oracle-runs/slot-a-native-u6f-02da-045
printf '%s  %s\n' \
  a7141f466256f2f139517982997c2afd5d774f6b07e5dfe0d548b1eba1e1bb65 \
  "$run/ev-transcript.txt" | sha256sum -c -
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude \
  src/recovered/slot_a_u6f_02da_decision.c \
  tests/recovered/check_slot_a_u6f_02da_native.c -o "$run/check_native"
"$run/check_native" "$run"
```

Run 045 agrees at all five post-entry stops. RAM above `0x9FFFF`,
the previous pathname conversion and DOS event, and the later `+031F`
comparison/retry/return are outside this comparison.
