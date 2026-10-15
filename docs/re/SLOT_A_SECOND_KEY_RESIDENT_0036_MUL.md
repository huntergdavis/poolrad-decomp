# Second-key resident caller `+0036` unsigned multiply

This gate continues the merged
[`+0033` AX-immediate stop](SLOT_A_SECOND_KEY_RESIDENT_0033_AX_IMMEDIATE.md)
from loaded `10D2:0036` through one `MUL WORD PTR SS:[BP-8]`,
stopping before `+0039`. The prior arithmetic helper remains
external. This is bounded instruction parity, not full-function or
complete second-key parity.

## Source and reached input

The required `deja "Poolrad RES_08AE 0036 MUL BP-8 second key"`
query found no indexed prior session. The earlier
[`unit 6F MUL note`](SLOT_A_NATIVE_U6F_1937_PRECALL.md) was reused
for the discipline of labeling MUL's undefined flags; it does not
supply this instruction's operand or output. We also inspected the
pinned DOSBox-X `src/cpu/instructions.h` `MULW` macro and its
`FillFlagsNoCFOF` helper before modeling the observed flag result.

The controlled slot-A run used DOS v1.3 START.EXE SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and 13,137-byte slot-A SAVGAMA.DAT SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
Original START file `0x8B2D..0x8B2F` and loaded
`10D2:0036..0038` both contain `F7 66 F8`, SHA-256
`441fb22cfbec274aaa06711221e3616183230798f65c630736ec0df065643d8c`.
At entry, BP=`3DA6h`, so SS:`[BP-8]` is SS:`3D9E` and contains
the little-endian WORD `00 00`. The reached multiplication is
`1770h × 0000h`.

The 14 CPU words below are ordered
`CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS`:

| Stop | CPU words | 640 KB RAM SHA-256 |
| --- | --- | --- |
| `10D2:0036` | `10D2 0036 14A0 1D16 1D16 3D9A 3DA6 1770 0005 7E40 0005 0001 0000 7206` | `420da85f0c5ed4da09e42dbe7f32443cb5f5b6b7135d3bf24532ed740c3a62f9` |
| `10D2:0039` | `10D2 0039 14A0 1D16 1D16 3D9A 3DA6 0000 0005 7E40 0000 0001 0000 7246` | same |

The product sets DX:AX=`0000:0000` and advances IP three bytes.
All 655,360 RAM bytes are unchanged. CF and OF are clear because
the high WORD is zero. ZF becomes set, PF stays set, and AF/SF stay
clear in this pinned DOSBox-X replay. ZF, PF, AF, and SF are
architecturally undefined for MUL; the C adapter mirrors the
inspected DOSBox-X `MULW` macro for the materialized input FLAGS:
ZF from AX zero, PF from parity across DX:AX, CF/OF from DX nonzero,
and prior AF/SF preserved. This backend-specific normalization is
not asserted as a general x86 contract.

## Bounded C parity

`pr_rec_res_08ae_0036_mul_local` reads the caller-mapped operand
WORD, computes a 32-bit unsigned product, updates DX:AX, and stops
at IP=`0039`. The ignored native-seeded harness maps the operand
to SS:`[BP-8]` and compares all 655,360 RAM bytes and 14 CPU words
under strict C11 warnings: **0/655,360 RAM and 0/14 CPU-word
mismatches**. The registered focused CTest includes the native zero
operand, a high-WORD nonzero product (defined CF/OF set), and a
nonzero low product that checks the backend parity normalization.
Private captures remain under ignored `resources/`.

The next disjoint gate remains `+0039..+003E`: `XOR DX,DX; ADD
AX,CX; ADC DX,BX`, stopping at `+003F` from its own native entry.

## Parity review

- **target_function:** reached `RES_08AE:+0036..+0038`, stopping
  before `+0039`.
- **source_candidates:** `pr_rec_res_08ae_0036_mul_local` models
  the single unsigned WORD multiply and the pinned emulator's
  materialized flag outcome.
- **signals:** exact original and loaded opcode; direct same-run
  operand and CPU/RAM stops; full-state C replay; synthetic
  high-WORD and nonzero-product tests; pinned emulator flag logic.
- **score and verdict:** 4/5 corroborating signal groups, **match**
  for this reached zero-operand instruction. Independent nonzero
  native confirmation remains open.
- **review_notes:** the live operand is zero, so synthetic tests
  exercise nonzero products without claiming native parity for those
  paths. Lazy flag state is not represented by the CPU struct; the
  model assumes the captured incoming FLAGS have been materialized.
  MUL's undefined flags are backend-specific, and the prior helper
  return is outside this gate.
