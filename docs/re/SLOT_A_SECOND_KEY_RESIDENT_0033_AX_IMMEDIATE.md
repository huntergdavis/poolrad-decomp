# Second-key resident caller `+0033` AX immediate

This gate continues the merged
[`+001F..+0033` conditional caller composition](SLOT_A_SECOND_KEY_RESIDENT_001F_0033_COMPOSED.md)
from loaded `10D2:0033` through one `MOV AX,1770h`, stopping before
the multiply instruction at `+0036`. The earlier arithmetic helper
return remains an external input; this gate earns no full-function or
complete second-key parity credit.

## Source and native stop

The required `deja "Poolrad RES_08AE 0033 MOV AX 1770 second key"`
query found no indexed prior session. The linked composition note
records the direct same-run `+0033` input state.

The controlled run used DOS v1.3 START.EXE SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and 13,137-byte slot-A SAVGAMA.DAT SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
Original START file `0x8B2A..0x8B2C` and loaded
`10D2:0033..0035` both contain `B8 70 17`, SHA-256
`4f179f9e4c0ecf3a250c696516c3df71d97efbae69a2692d79aaa74cec31a18f`.
The next unexecuted bytes are `F7 66 F8` at `+0036`.

The 14 CPU words below are ordered
`CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS`:

| Stop | CPU words | 640 KB RAM SHA-256 |
| --- | --- | --- |
| `10D2:0033` | `10D2 0033 14A0 1D16 1D16 3D9A 3DA6 7E40 0005 7E40 0005 0001 0000 7206` | `420da85f0c5ed4da09e42dbe7f32443cb5f5b6b7135d3bf24532ed740c3a62f9` |
| `10D2:0036` | `10D2 0036 14A0 1D16 1D16 3D9A 3DA6 1770 0005 7E40 0005 0001 0000 7206` | same |

AX changes from `7E40h` to `1770h`; IP advances three bytes.
All other CPU words, including flags, are unchanged, and the
complete RAM images are byte-identical.

## Bounded C parity

`pr_rec_res_08ae_0033_load_ax` sets AX to the source immediate and
stops at IP `0036`. The ignored native-seeded harness compares all
655,360 RAM bytes and all 14 CPU words under strict C11 warnings:
**0/655,360 RAM and 0/14 CPU-word mismatches**. The registered
focused CTest checks the native values and a synthetic AX/flags
input. Private captures remain under ignored `resources/`.

Two distinct follow-on spans are queued: `+0036..+0038` is the
unsigned `MUL WORD PTR [BP-8]`, stopping at `+0039`; and
`+0039..+003E` is `XOR DX,DX; ADD AX,CX; ADC DX,BX`, stopping at
`+003F`. Each requires its own native input state and parity gate.

## Parity review

- **target_function:** reached `RES_08AE:+0033..+0035`, stopping
  before `+0036`.
- **source_candidates:** `pr_rec_res_08ae_0033_load_ax` models
  only `MOV AX,1770h`.
- **signals:** exact original and loaded three-byte opcode; direct
  same-run CPU and full RAM stops; full-state C replay; synthetic
  alternate AX and flags.
- **score and verdict:** 4/5 corroborating signal groups, **match**
  for this single instruction. Independent second-run confirmation
  remains open.
- **review_notes:** AX is overwritten regardless of its incoming
  value. The instruction has no flag or RAM effects. Neither the
  following multiply nor the preceding helper return is modeled here.
