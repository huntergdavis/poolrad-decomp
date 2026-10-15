# Second-key resident caller `+0039` arithmetic gate

This bounded gate continues the preceding
[`+0036` multiply](SLOT_A_SECOND_KEY_RESIDENT_0036_MUL.md) from loaded
`10D2:0039` through `XOR DX,DX; ADD AX,CX; ADC DX,BX`, stopping before
`+003F MOV CX,AX`. It is instruction parity, not full-function or
complete second-key parity.

## Source and native stops

The required `deja "Poolrad RES_08AE +0039 XOR DX DX ADD AX CX ADC DX BX key route native parity"`
query found no indexed prior session. The preceding multiply note
supplies the same-run `+0039` entry and full-state harness pattern;
the exit below was captured directly from the live native replay.

The controlled slot-A run used DOS v1.3 START.EXE SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and 13,137-byte slot-A SAVGAMA.DAT SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
Original START file `0x8B30..0x8B35` contains `31 D2 03 C1 13 D3`,
SHA-256 `58735deab956144c433f8abb1d8f0d57d5e639503f1c6a4495192f89f54d0491`.

The 14 CPU words are ordered `CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS`:

| Stop | CPU words | 640 KB RAM SHA-256 |
| --- | --- | --- |
| `10D2:0039` | `10D2 0039 14A0 1D16 1D16 3D9A 3DA6 0000 0005 7E40 0000 0001 0000 7246` | `420da85f0c5ed4da09e42dbe7f32443cb5f5b6b7135d3bf24532ed740c3a62f9` |
| `10D2:003F` | `10D2 003F 14A0 1D16 1D16 3D9A 3DA6 7E40 0005 7E40 0005 0001 0000 7206` | same |

All 655,360 RAM bytes are unchanged. XOR clears DX and carry. ADD
sets AX=`7E40h` with no carry; ADC then sets DX=`0005h` and materializes
FLAGS=`7206h`. The intermediate AF after XOR is undefined, but ADD
and ADC both define it. All arithmetic flags at the `+003F` stop are
defined. The C adapter computes the final ADC flags and preserves
unrelated FLAGS bits. It also models carry propagation from ADD for
synthetic inputs; those inputs are unit checks, not additional native
path claims.

## Bounded C parity

`pr_rec_res_08ae_0039_accumulate` models the three instructions and
stops at IP=`003F`. The ignored native-seeded harness compares all
655,360 RAM bytes and 14 CPU words under strict C11 warnings:
**0/655,360 RAM and 0/14 CPU-word mismatches**. The focused CTest
checks the reached input plus low-WORD carry, ADC overflow/auxiliary
carry, and signed high-WORD output. Private captures remain under
ignored `resources/`.

The next disjoint gate begins at `+003F`; the reached conditional
successor remains to be measured after that caller continuation.

## Parity review

- **target_function:** reached `RES_08AE:+0039..+003E`, stopping before
  `+003F`.
- **source_candidates:** `pr_rec_res_08ae_0039_accumulate` models XOR,
  ADD, and ADC at this call site.
- **signals:** exact original opcode bytes; same-run entry and native
  exit CPU/RAM; full-state C replay; focused synthetic carry/flag tests.
- **score and verdict:** 4/5 corroborating signal groups, **match** for
  this reached arithmetic gate. Other native arithmetic inputs remain
  unobserved.
- **review_notes:** this is a bounded instruction slice with no
  strict-function or playable-game credit. The C adapter assumes
  materialized input FLAGS and does not model the next MOV or branch.
