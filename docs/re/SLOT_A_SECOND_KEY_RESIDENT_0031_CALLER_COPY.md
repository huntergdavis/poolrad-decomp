# Second-key resident caller `+0031` copy

This gate continues the direct same-run
[`+002A` far-call and `+002F` return trace](SLOT_A_SECOND_KEY_RESIDENT_002A_CALLER_RETURN.md)
from loaded `10D2:0031` to `10D2:0033`. The arithmetic helper has
already returned; its body and outputs are external to this model.

## Source and native stops

The required `deja "Poolrad RES_08AE 0031 second key MOV BX DX arithmetic
return"` query found no indexed prior session. The linked return note
supplies the same-run `+0031` start state.

Input DOS v1.3 START.EXE SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
GAME.OVR SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
the 13,137-byte slot-A `SAVGAMA.DAT` SHA-256 is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
Original START file `0x8B28..0x8B29` and loaded `10D2:0031..0032`
both contain `8B DA`, SHA-256
`f0106d6a52b9460e243bb6955fb007088d0cc32710697949eddb6105cfdf7f3e`.
The instruction is `MOV BX,DX`; the unexecuted next instruction starts
at `+0033` (`B8 70 17`, `MOV AX,1770h`).

The 14 CPU words are ordered
`CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS`:

| Stop | CPU words | 640 KB RAM SHA-256 |
| --- | --- | --- |
| `10D2:0031` | `10D2 0031 14A0 1D16 1D16 3D9A 3DA6 7E40 0005 7E40 0005 0001 0000 7206` | `420da85f0c5ed4da09e42dbe7f32443cb5f5b6b7135d3bf24532ed740c3a62f9` |
| `10D2:0033` | `10D2 0033 14A0 1D16 1D16 3D9A 3DA6 7E40 0005 7E40 0005 0001 0000 7206` | same |

The complete RAM images are byte-identical. BX was already `0005`,
equal to DX, so this single native transition establishes the reached
instruction boundary and lack of RAM/flag effects but cannot by
itself distinguish copying DX from preserving BX. The original
opcode and a synthetic `BX=1234h`, `DX=BEEFh` case distinguish those
semantics.

## Bounded C parity

`pr_rec_res_08ae_0031_copy_dx_to_bx` copies DX to BX and advances IP
to `0033`; it leaves all other CPU words and caller-owned memory
alone. The ignored native-seeded harness compares all 655,360 RAM
bytes and all 14 CPU words under strict C11 warnings:
**0/655,360 RAM and 0/14 CPU-word mismatches**. The registered
focused CTest checks both the native values and the unequal-operand
synthetic case. Private captures remain under ignored `resources/`.

The instruction starting at `+0033` and any composition through this
caller span remain separate gates.

## Parity review

- **target_function:** reached `RES_08AE:+0031..+0032`, stopping
  before `+0033`.
- **source_candidates:** `pr_rec_res_08ae_0031_copy_dx_to_bx` covers
  only `MOV BX,DX`.
- **signals:** exact original and loaded opcode; direct same-run CPU
  stops and full RAM equality; full-state C replay; unequal-operand
  synthetic check.
- **score and verdict:** 4/5 corroborating signal groups, **match**
  for this one instruction. An independent second native run remains
  open.
- **review_notes:** the observed `BX=DX=0005` means native register
  equality alone is insufficient to prove a copy. The decoded opcode
  and synthetic check supply that distinction. This is bounded
  instruction parity, with zero full-function credit.
