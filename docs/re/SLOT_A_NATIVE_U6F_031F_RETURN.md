# Saved slot-A unit 0x6F error-5 return

This bounded continuation starts at loaded `2A8E:031F`, the reached
error-5 branch target after the merged
[IOResult decision model](SLOT_A_NATIVE_U6F_02DA_DECISION.md). The saved
path falls through both local conditional branches, unwinds the frame, and
returns far to `2A8E:19EA`. That return is the first stable boundary after
`+031F`. Neither the zero-result route, the other-error retry route, nor
the caller's next instruction is translated here. This is no full unit or
one-Up parity claim.

Before implementation, `deja "poolrad slot A unit 6F 031F error 5 retry
return native CPU RAM"` found no indexed match. The merged decision note
supplied the native `+031F` handoff. The same ignored native run then
directly captured every subsequent instruction stop and the far-return
target.

## Original and loaded instruction proof

Run `resources/oracle-runs/slot-a-native-u6f-02da-045` remains ignored.
Its pinned inputs are START.EXE 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
GAME.OVR 206,105 bytes,
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
slot-A SAVGAMA.DAT 13,137 bytes,
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`;
and heavy DOSBox-X 47,259,176 bytes,
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

Original `GAME.OVR[0x1E305..0x1E314]` and loaded
`2A8E:031F..032E` match exactly: 16 bytes, seven instructions, two
conditional branches, SHA-256
`b43be1744a6c1c56864f873d37e798884a0bf69bd0b7bcd72f0392602899dc83`.
The instruction sequence is: compare SS:[BP−2] with zero at `+031F`;
JE `+032B` at `+0323`; compare the same word with 5 at `+0325`;
JNE `+02B5` at `+0329`; MOV SP,BP at `+032B`; POP BP at `+032D`;
and RETF at `+032E`.

## Reached native state

Across all stops, CS=`2A8E`, DS=`14A0`, ES=`1D16`, FS=GS=`0000`,
SS=`1D16`, AX=`0005`, BX=`35A0`, CX=`0000`, DX=`3526`,
SI=`36BB`, and DI=`3531`. The row values complete the 16-field CPU
tuple. Every captured `0x00000..0x9FFFF` RAM image has the same SHA-256:
`85d06b2407f87dbad815dd70d3b3d5efe9950d5eb14e66fa1a0152a8f68187f6`.
No byte in that 655,360-byte range changed along the continuation.

| Stop IP | SP | BP | FLAGS | Observed step from preceding stop |
| --- | --- | --- | --- | --- |
| `031F` | `35B0` | `37B2` | `7246` | handoff from `+02E7` |
| `0323` | `35B0` | `37B2` | `7206` | CMP 5,0 sets ZF=0 |
| `0325` | `35B0` | `37B2` | `7206` | JE not taken |
| `0329` | `35B0` | `37B2` | `7246` | CMP 5,5 sets ZF=1 |
| `032B` | `35B0` | `37B2` | `7246` | JNE not taken |
| `032D` | `37B2` | `37B2` | `7246` | SP becomes BP |
| `032E` | `37B4` | `3DCA` | `7246` | saved BP popped |
| `19EA` | `37B8` | `3DCA` | `7246` | far return completed |

The private stack at SS:`37B2` contains saved BP=`3DCA`, far return
IP=`19EA` at `+37B4`, and CS=`2A8E` at `+37B6`. The earlier
SS:[BP−2] result word is linear `0x20910`, correcting the numeric typo
in the merged decision note. The previous native/C parity and hashes
are unchanged. The ignored eight-row numeric EV transcript is
`resources/oracle-runs/slot-a-native-u6f-02da-045/ev-transcript-031f.txt`,
SHA-256 `e026efbbe7030d5c3042ac99608b5dc4b6d2110386b8a8001a57a65e3ebe3ebd`.

## C boundary and verification

`pr_slot_a_u6f_031f_step` uses the existing full-state CPU/RAM type and
executes one reached instruction per call. It requires the observed
error-5 word at each CMP, stops before either unobserved branch, and
accepts RETF only to the observed `2A8E:19EA` frame. A missing RAM byte,
unexpected CS/IP, or unobserved result returns `BOUNDARY` before that
instruction mutates state. Word access wraps each 16-bit segment offset
and the 20-bit physical address. Synthetic tests exercise the saved
return, zero/other-result boundaries, altered branch flags, stack offset
wrap, missing memory, and wrong return target.

To reproduce, prepare and launch the hash-pinned private slot-A run as in
the linked decision note. Pause at `2A8E:031F`, arm
`BP 2A8E:0323`, `BP 2A8E:0325`, `BP 2A8E:0329`,
`BP 2A8E:032B`, `BP 2A8E:032D`, `BP 2A8E:032E`, and
`BP 2A8E:19EA`. At each stop issue
`EV CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS` and
`MEMDUMPBIN 0000 0000 A0000`. Keep the dumps and normalized EV rows
under ignored `resources/`; no original bytes or captures are tracked.

For this pinned run, the source-only verifier reads the ignored EV file
and RAM snapshots rather than hardcoding expected CPU states:

```sh
run=resources/oracle-runs/slot-a-native-u6f-02da-045
printf '%s  %s\n' \
  e026efbbe7030d5c3042ac99608b5dc4b6d2110386b8a8001a57a65e3ebe3ebd \
  "$run/ev-transcript-031f.txt" | sha256sum -c -
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude \
  src/recovered/slot_a_u6f_031f_return.c \
  tests/recovered/check_slot_a_u6f_031f_native.c -o "$run/check_031f_native"
"$run/check_031f_native" "$run"
```

All 16 CPU fields and all 655,360 captured RAM bytes agree after each
of seven C steps. RAM above `0x9FFFF` and execution at `+19EA` are
outside this parity check.
