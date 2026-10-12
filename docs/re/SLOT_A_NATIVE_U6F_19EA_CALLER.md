# Saved slot-A unit 0x6F caller branch after error-5 return

This bounded continuation begins at loaded `2A8E:19EA`, immediately
after the saved error-5 far return in the merged
[return note](SLOT_A_NATIVE_U6F_031F_RETURN.md). The reached caller
loads one stack byte into AL, compares it with another, falls through
the unequal JE, and jumps to `2A8E:1937`. The equal branch at `+19F2`
and code at `+1937` remain outside the C model. The earlier unit zero
and other-error routes, enclosing game routine, and C-native one-Up
replay are not established by this slice.

Before implementation, `deja "poolrad slot A unit 6F caller 19EA error 5
return next branch external call native"` found no indexed match. The
merged return note supplied the reached `+19EA` handoff; the same
ignored native run directly captured four subsequent instruction stops.

## Pinned original and loaded code

Ignored run `resources/oracle-runs/slot-a-native-u6f-02da-045` uses
START.EXE 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
GAME.OVR 206,105 bytes,
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
slot-A SAVGAMA.DAT 13,137 bytes,
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`;
and heavy DOSBox-X 47,259,176 bytes,
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

The preceding original/loaded `+19E5..+19E9` five-byte span has
SHA-256 `0c465873810bce9abb342904f1501d937927a6cfd6f99387927b34267ad7a3e3`.
It includes `PUSH CS` at `+19E6` and a near `CALL +02AE` at `+19E7`,
establishing why unit `0x6F:+02AE` uses RETF to resume at `+19EA`.
The returned frame in the prior note independently confirms CS:IP
`2A8E:19EA`.

Original `GAME.OVR[0x1F9D0..0x1F9DC]` and loaded
`2A8E:19EA..19F6` match exactly: 13 bytes, four instructions, one
conditional and one unconditional branch, SHA-256
`f8167ce0d816f623861fd01e72891f85d08215eece2fea53846ed0cba0c3b5a4`.
The instructions are MOV AL,SS:[BP−01C9] at `+19EA`; CMP
AL,SS:[BP−0212] at `+19EE`; JE `+19F7` at `+19F2`; and JMP
`+1937` at `+19F4`.

## Reached native state

At all five direct stops CS=`2A8E`, DS=`14A0`, ES=`1D16`,
FS=GS=`0000`, SS=`1D16`, SP=`37B8`, BP=`3DCA`, BX=`35A0`,
CX=`0000`, DX=`3526`, SI=`36BB`, and DI=`3531`.
The rows complete the 16-field C CPU tuple. Every captured 640 KiB
RAM image has SHA-256
`85d06b2407f87dbad815dd70d3b3d5efe9950d5eb14e66fa1a0152a8f68187f6`;
there is no byte change in `0x00000..0x9FFFF`.

| Stop IP | AX | FLAGS | Reached effect |
| --- | --- | --- | --- |
| `19EA` | `0005` | `7246` | far-return handoff |
| `19EE` | `0001` | `7246` | stack byte 1 loaded into AL |
| `19F2` | `0001` | `7293` | byte comparison, ZF=0 |
| `19F4` | `0001` | `7293` | JE not taken |
| `1937` | `0001` | `7293` | near jump completed |

With SS=`1D16` and BP=`3DCA`, the directly inspected first operand
is SS:`3C01`, linear `0x20D61`, byte 1. The compared second operand
is SS:`3BB8`, linear `0x20D18`, byte 6. Thus the native CMP is AL−6,
and its recorded FLAGS=`7293` include ZF=0. The ignored normalized
five-row EV transcript is
`resources/oracle-runs/slot-a-native-u6f-02da-045/ev-transcript-19ea.txt`,
SHA-256 `97c4f5df7bebc7e38c9ea18c64cd48d3dc9fb632ae6eed33f95cbb79de89f90f`.

## C boundary and reproduction

`pr_slot_a_u6f_19ea_step` uses the existing full-state CPU/RAM type.
Each call executes one reached instruction. The two stack reads use
16-bit wrapped BP offsets and 20-bit wrapped physical addresses. The
CMP computes all six 8086 arithmetic flags and preserves other flag
bits. A missing operand byte, wrong CS/IP, equal comparison at `+19F2`,
or terminal `+1937` returns `BOUNDARY` before that instruction changes
state. Synthetic tests exercise the saved unequal route, equal boundary,
flags, RAM preservation, and address wrap.

To reproduce, prepare and launch the hash-pinned private slot-A run as
in the linked return note. Pause at `2A8E:19EA` after the far return;
arm `BP 2A8E:19EE`, `BP 2A8E:19F2`, `BP 2A8E:19F4`, and
`BP 2A8E:1937`. At each stop issue
`EV CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS` and
`MEMDUMPBIN 0000 0000 A0000`; keep all dumps and normalized EV rows
under ignored `resources/`.

For the pinned run, the source-only verifier reads the ignored
transcript and RAM images, without hardcoded expected CPU values:

```sh
run=resources/oracle-runs/slot-a-native-u6f-02da-045
printf '%s  %s\n' \
  97c4f5df7bebc7e38c9ea18c64cd48d3dc9fb632ae6eed33f95cbb79de89f90f \
  "$run/ev-transcript-19ea.txt" | sha256sum -c -
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude \
  src/recovered/slot_a_u6f_19ea_caller.c \
  tests/recovered/check_slot_a_u6f_19ea_native.c -o "$run/check_19ea_native"
"$run/check_19ea_native" "$run"
```

All 16 CPU fields and all 655,360 captured RAM bytes agree after
each of four C steps. RAM above `0x9FFFF` and execution at `+1937`
are outside this parity comparison.
