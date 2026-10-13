# Saved slot-A unit 0x6F caller setup before the next external call

This bounded continuation starts at loaded `2A8E:1937`, the reached
unequal-branch target after the merged
[caller decision](SLOT_A_NATIVE_U6F_19EA_CALLER.md). It executes the
local stack-byte increment, pointer calculations, arithmetic, and ten
word pushes. It stops before the first external call at `2A8E:1965`.
The called helper, later caller behavior, enclosing unit, and composed
C-native one-Up replay remain outside this parity claim.

Before implementation, `deja "poolrad slot A unit 6F caller 1937 saved
branch external call native CPU RAM"` found no indexed match. The merged
caller note supplied the reached `+1937` CPU/RAM handoff. The same
ignored native run directly captured key intermediate stops and the
pre-call boundary.

## Pinned code and inputs

Ignored run `resources/oracle-runs/slot-a-native-u6f-02da-045` uses
START.EXE 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
GAME.OVR 206,105 bytes,
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
slot-A SAVGAMA.DAT 13,137 bytes,
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`;
and heavy DOSBox-X 47,259,176 bytes,
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

Original `GAME.OVR[0x1F91D..0x1F94A]` equals loaded
`2A8E:1937..1964`: 46 bytes, 21 instructions, no local branches,
SHA-256
`9a95c616a9dc9b6e71a6f840a3b9bd791695dd674db669b3c6eee8f85d4b16e7`.
The next instruction at `+1965` is a far call to `131C:0B0B`; this
model stops before its return-frame push or callee effects.

## Reached native CPU and RAM

At every listed stop CS=`2A8E`, DS=`14A0`, ES=`1D16`, FS=GS=`0000`,
SS=`1D16`, BP=`3DCA`, BX=`35A0`, CX=`0000`, and SI=`36BB`.
The rows plus those constants are the complete 16-field C CPU tuple.
RAM hashes cover linear `0x00000..0x9FFFF` (655,360 bytes).

| Stop IP | SP | AX | DX | DI | FLAGS | RAM SHA-256 |
| --- | --- | --- | --- | --- | --- | --- |
| `1937` | `37B8` | `0001` | `3526` | `3531` | `7293` | `85d06b2407f87dbad815dd70d3b3d5efe9950d5eb14e66fa1a0152a8f68187f6` |
| `193B` | `37B8` | `0001` | `3526` | `3531` | `7203` | `e36bc609198504fa9c0c951efc714677072130d8e3ea5fde2125a80f480cf871` |
| `1947` | `37B0` | `0001` | `3526` | `3AB6` | `7203` | `c3aabdf3e6faa2a54e1d2ad48ea01906abf29691e53b9abda219f0c13171a7b6` |
| `194D` | `37B0` | `0002` | `3526` | `3AB6` | `7246` | same as `1947` |
| `1952` | `37B0` | `0052` | `0000` | `3AB6` | `7202` | same as `1947` |
| `1958` | `37B0` | `0052` | `0000` | `3CAB` | `7202` | same as `1947` |
| `1965` | `37A4` | `0052` | `0000` | `15E7` | `7202` | `d5f23fa401e73c15ae48769e5da4275d9757f48ed3dc5ffc42b5fa47206a963d` |

The `INC` at `+1937` changes SS:`3C01`, linear `0x20D61`, from 1 to 2.
Across the whole span exactly 21 net RAM bytes change: that byte and
20 stack bytes. At `+1965`, SP=`37A4`. The ten stack words from
SS:`37A4` upward are, in order, `15E7`, `2A8E`, `3BB6`, `1D16`,
`3CAB`, `1D16`, `3AB6`, `1D16`, `39B6`, `1D16`. This order matches the
interleaved PUSH DI, PUSH CS, and PUSH SS instructions.

The saved `XOR AH,AH` yields FLAGS=`7246`; saved unsigned
`2 × 0x29` yields AX=`0052`, DX=`0000`, FLAGS=`7202` after MUL.
CF and OF are clear because the high word is zero. Other arithmetic
flags after MUL are undefined by the instruction and normalized to the
observed zero values in this bounded model. The ignored seven-row EV
transcript at
`resources/oracle-runs/slot-a-native-u6f-02da-045/ev-transcript-1937.txt`
has SHA-256
`5b4aaaf1861f94db20dab1b0384bd4d146a8e132048a0661815f64b3882f4f22`.

## C boundary and reproduction

`pr_slot_a_u6f_1937_step` uses the existing full-state CPU/RAM type.
Each call executes one of the 21 instructions, including 8086 byte
INC flags with CF preserved, 16-bit effective addresses, unsigned
word multiplication, and separately wrapped word-byte stack writes.
Each push validates both destination bytes before changing RAM or SP.
An unavailable byte, unexpected CS/IP, or the terminal `+1965` returns
`BOUNDARY` before that instruction mutates state. Synthetic tests cover
the saved arithmetic and complete ten-word push order, RAM isolation,
INC overflow, offset and physical wrap, and missing-memory preflight.

To reproduce, prepare and launch the hash-pinned private slot-A run as
in the linked caller note. Pause at `2A8E:1937`; arm
`BP 2A8E:193B`, `BP 2A8E:1947`, `BP 2A8E:194D`,
`BP 2A8E:1952`, `BP 2A8E:1958`, and `BP 2A8E:1965`.
At each stop issue
`EV CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS` and
`MEMDUMPBIN 0000 0000 A0000`; keep the dumps and normalized EV rows
under ignored `resources/`.

For the pinned run, the source-only verifier steps between the captured
stops and reads the ignored transcript and RAM snapshots:

```sh
run=resources/oracle-runs/slot-a-native-u6f-02da-045
printf '%s  %s\n' \
  5b4aaaf1861f94db20dab1b0384bd4d146a8e132048a0661815f64b3882f4f22 \
  "$run/ev-transcript-1937.txt" | sha256sum -c -
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude \
  src/recovered/slot_a_u6f_1937_precall.c \
  tests/recovered/check_slot_a_u6f_1937_native.c -o "$run/check_1937_native"
"$run/check_1937_native" "$run"
```

All 16 CPU fields and all 655,360 captured RAM bytes agree at each
of six post-entry stops. RAM above `0x9FFFF` and the external call
at `+1965` are outside this comparison.
