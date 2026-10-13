# Saved slot-A `+196A` resident call: live return and output writer

The second saved-slot-A call at loaded overlay `2A8E:196A` reaches resident
`1026:0356` and returns to `2A8E:196F`. It is the continuation after the
merged [`+1937` pre-call setup](SLOT_A_NATIVE_U6F_1937_PRECALL.md) and
[`+1965` SYSTEM copy](SLOT_A_NATIVE_U6F_1965_SYSTEM_COPY.md). The original
live call changes a local scratch string and output string, then returns with
the output far pointer retained for the next caller instruction. The output
length remains 8; seven of its eight payload bytes change. Window 0 has no
net RAM change across this bounded call.

The output instruction was pinpointed by a **controlled re-execution** of
the same call inside the paused native DOSBox-X run. I restored exactly the
30 RAM bytes changed by the original call, verified a full 655,360-byte RAM
hash equal to the original `+196A` stop, reset all 16 CPU fields to that
stop, and ran only `+196A..+196F` again. Its final RAM and CPU tuple exactly
match the original live return. This is direct instruction evidence for the
bounded call, with the original live run providing the saved-route endpoint.
It is not a fresh end-to-end replay, C-native one-Up parity, or a new
game-function count.

Before this trace, `deja "poolrad slot A native 2A8E 196A resident 1026
0356 return first writer"` returned no exact match. `deja 196A` recalled
the merged `+1965` boundary and the existing
[resident string audit](RESIDENT_STRING_0356.md). I used those as leads,
then independently checked the original and loaded caller and callee bytes,
the guest stack, the call stops, and full-RAM snapshots.

## Pinned code and call boundary

The ignored run is `resources/oracle-runs/slot-a-native-u6f-196a-048`.
Its original DOS v1.3 inputs are `START.EXE` 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` 206,105 bytes,
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
and slot-A `SAVGAMA.DAT` 13,137 bytes,
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private heavy DOSBox-X executable is 47,259,176 bytes, SHA-256
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

| Original or loaded span | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR[0x1F950..0x1F954]`, unit `0x6F:+196A` caller | 5 bytes | `965606c688f124e8166f41f356e580d9d2d0897521f1a7db054b55733834822d` |
| Loaded `2A8E:196A..196E` caller | 5 bytes | `f2fd40cc31e4aa01a804aac27d0a12ab79d65949e939e63564877d0448cc6dee` |
| `START.EXE[0x083AD..0x0847B]`, resident `0802:0356..0424` | 207 bytes / 82 instructions | `ab5aee9717d1a2e5f472234bc2198528c02792bec1098de0dbb7be0fef4ea889` |
| Loaded `1026:0356..0424` | 207 bytes | `2d2a3260b63ef4f99decb51f44ec543a83d5c1ec4f8c854da949a376e56b722e` |
| Original resident final far call, file `0x08471..0x08475`, offset `+041A` | 5 bytes | `6d7fadfa225a8fcbe50f8aa9bd23a82b01605ad36537649b8c460d85459776a1` |
| Loaded final call `1026:041A..041E` | 5 bytes | `74e04ab27a444791cf75ab6dda2cb5081e6b6378829ce6fe21620a6baa3156f1` |
| Original/loaded SYSTEM Copy, file `0x0B79B..0x0B7BE`, loaded `131C:0B25..0B48` | 36 bytes | `009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118` |

The caller's far target is original `0802:0356`, live `1026:0356`.
The resident body has exactly 12 changed loaded bytes: six far-call
segment words at body-relative offsets `0x19`, `0x2D`, `0x83`, `0x8E`,
`0xA9`, and `0xC7`, each relocated from `0AF8` to `131C`. Its final
far call at `+041A` targets original `0AF8:0B25`, live `131C:0B25`.
The SYSTEM Copy body is byte-identical between original and loaded RAM.
Its one-byte `STOSB` at `+0B3D` hashes
`bceef655b5a034911f1c3718ce056531b45ef03b4c7b1f15629e867294011a7d`;
its two-byte `REP MOVSB` at `+0B42` hashes
`00b4f1541a5abd60fc5e6d7eaa5a807d3c34cdda45627839b36c91f79340c935`.

At the live second caller stop, DS=`14A0`, SS=`1D16`, SP=`37A8`.
The three far pointers on the stack are needle `1D16:3BB6`, source
`1D16:3CAB`, and output `1D16:3AB6`. The far call pushes return
`2A8E:196F` at SS:`37A4..37A7`, and the resident's first instruction,
`PUSH BP` at `1026:0356`, changes physical `0x20902..0x20903`.
The resident returns through `RETF 8` to `2A8E:196F`, SP=`37B0`,
retaining output `1D16:3AB6` at the stack top. The live return CPU tuple
is `CS:IP=2A8E:196F`, `DS=14A0`, `ES=SS=1D16`, `FS=GS=0000`,
`BP=3DCA`, `AX=0008`, `BX=3490`, `CX=0000`, `DX=14A0`,
`SI=36AB`, `DI=3ABF`, `FLAGS=7246`.

## Value-changing writes

The first `+196A` hit in the private run is a control call; the second hit
is the reached continuation from `+1937`. Their CPU tuples are equal, while
the byte at SS:`3C01` is 1 and 2 respectively. The full-RAM comparison of
the second hit and its return has exactly 30 net changed bytes, in these
physical spans: `0x205F6..0x205FF` (10), `0x20700..0x20703` (4),
`0x20806..0x2080A` (5), `0x20902..0x20905` (4), and
`0x20C18..0x20C1E` (7). Fourteen are local stack bytes, five are scratch
bytes, four are call-frame/prologue stack bytes, and seven are output
payload bytes. The changed sets are checked against all 655,360
RAM bytes, not just these ranges.

The first *resident-owned* value-changing instruction is `PUSH BP` at
`1026:0356`. The first changed non-stack bytes in the observed path are
scratch payload physical `0x20806..0x2080A`: SYSTEM Copy's `REP MOVSB`
at `131C:0B42`, reached from resident `+036C`. Its preceding `STOSB`
at `+0B3D` stores the same length value 8 and changes no RAM byte.

For the **final output copy**, the controlled re-execution stopped at
`1026:041A` with the nested call arguments on SS:`3494..349D`:
cap `255`, destination `1D16:3AB6`, source `1D16:36A2`.
The far-call return frame at SS:`3490..3493` is `1026:041F`.
At `131C:0B3D`, ES:DI=`1D16:3AB6`; `STOSB` advances DI to `3AB7`
but changes zero bytes because the output length is already 8.
At `131C:0B42`, `REP MOVSB` copies eight payload bytes. The stop at
`+0B44` shows CX=`0`, SI=`36AB`, DI=`3ABF`, and exactly seven changed
output bytes at physical `0x20C18..0x20C1E` (output offsets `+2..+8`).
The first payload byte already matched. The output's nine-byte Pascal
span then equals both the scratch source and the selected source span;
no payload values are published. The nested call returns to `1026:041F`
with SP=`349E`, then the resident returns to `2A8E:196F` with SP=`37B0`.

| Ignored RAM stop | SHA-256 of 655,360 bytes |
| --- | --- |
| Original second `2A8E:196A`; independently restored pre-call RAM | `4e25d55dfe20735950fb26c9115b5ec2d8d59990e1b0c62dc4b4173207c17f17` |
| Original `1026:0356` | `67f714ede36946adcbdc3c9bef3ec206ab2d0b91cad2f2d666b7e6cc52389a77` |
| Original first scratch Copy before/after `+0B3D` and `+0B3E` | `b479b9355db3e237e9149385be01bd901dac6b2a44f0cb497dd34d5e5f00c59a` |
| Original first scratch Copy after `+0B42`, stop `+0B44` | `4191e7da295ad18c4d51a628baff25ba0abe040a690eba1c6612101f951c9fd3` |
| Controlled final call before nested far call `1026:041A` | `d05a25d10f718600cbbfbc74baa7b45ae2f8f4d30b84a92804dac4ea4f59e98e` |
| Controlled final Copy before/after `STOSB`, stops `+0B3D`/`+0B3E` | `0f6ccc9f2def1e647eb7c319585c8805d6492f1ab44f144f37bee2cdcf2c8a67` |
| Controlled after `REP MOVSB`, `+0B44` and `1026:041F`; original live `2A8E:196F` | `1baad8fc261b2bbf80d5434e34ea1637e2c2ced7985aff77a706deb113cbe816` |

The ignored ten-row original live CPU transcript hashes
`1ec062b5e5bddd5e7811133516a723d1d2bec9a0c68fe9bd02933185ce768309`;
the six-row controlled transcript hashes
`2e12550d09d8d9048a810a4ea7bbb940965eb9dc2f2ffecef58a0337cb7baeaf`.
The optional checker validates these, every individual snapshot hash,
all code hashes and relocations, the call frames, the exact changed-address
sets, the same-value length stores, and full-RAM equality at the return:

```sh
python3 tests/recovered/check_slot_a_u6f_196a_resident_native.py \
  resources/oracle-runs/slot-a-native-u6f-196a-048
```

## Reproduction boundary

Prepare a new ignored run with `tools/oracle/prepare_run.sh`, launch it
with the hash-pinned heavy DOSBox-X through `tools/oracle/launch.sh` under
Xvfb, and pause at the visible saved-slot prompt. Arm `BP 2A8E:196A`,
resume, and select A. The second `+196A` hit is the reached caller used
here. At each listed guest stop, record
`EV CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS` and
`MEMDUMPBIN 0000:0000 A0000`, moving the private `MEMDUMP.BIN` to its
stop-labeled name before continuing. The first live trace stopped at
`1026:0356`, `+0357`, `+036C`, SYSTEM `131C:0B3D`, `+0B3E`, `+0B44`,
resident `+0371`, and caller `2A8E:196F`.

The controlled writer check starts from that paused live return. It uses
debugger `SM` to restore only bytes that differ from the ignored pre-call
RAM snapshot; the actual byte values stay private. Require a full-RAM SHA
equal to the original `+196A` image, then use debugger `SR` to restore the
recorded pre-call CPU tuple. Stop at `1026:041A`, then at SYSTEM
`131C:0B3D`, `+0B3E`, `+0B44`, resident `1026:041F`, and caller
`2A8E:196F`. One later fresh replay could not arm a prompt breakpoint
cleanly; it contributed no evidence to this note. The original live route
and controlled same-state re-execution are therefore distinguished above.

No save, executable, RAM image, emulator trace, screenshot, game string,
or extracted game bytes are tracked in this PR.
