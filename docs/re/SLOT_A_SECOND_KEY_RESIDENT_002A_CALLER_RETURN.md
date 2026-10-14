# Second-key resident far call and caller successor

This gate continues the measured later-iteration
[`RES_08AE:+001F..+0029` caller prefix](SLOT_A_SECOND_KEY_RESIDENT_001F_CALLER_PREFIX.md)
from `10D2:002A` through the far-call entry, then models the one reached
caller instruction at `+002F` after the arithmetic helper returns. The
arithmetic helper's body and outputs are external to this C adapter.

## Source and native run

The required `deja` search for the `+002A` return gate found no indexed
prior session. The linked caller-prefix note supplies the earlier
instruction mapping and the same-process `+002A` starting capture; all
stops below came from a direct continuation of that `+002A` stop.

Input START.EXE SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Original START file bytes at `0x8B21..0x8B25` are
`9A 5B 0A F8 0A`, SHA-256
`9f951a579c4cddf5ec24befa4e754494529b2d8188813195acb0eccc4d92cdbc`.
The fresh `10D2:002A` RAM image contains relocated
`9A 5B 0A 1C 13`: `CALL FAR 0AF8:0A5B` in the original image,
`131C:0A5B` in this run. The next loaded bytes are `8B C8`,
`MOV CX,AX` at `10D2:002F`. The C call adapter receives the loaded
target segment rather than assuming a fixed relocation.

The 14 CPU words below are in DOSBox debugger order:
`CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS`.

| Stop | CPU words | 640 KB RAM SHA-256 |
| --- | --- | --- |
| before far call `10D2:002A` | `10D2 002A 14A0 1D16 1D16 3D9A 3DA6 0001 0005 7E40 0000 0001 3DA2 7246` | `770d855ea3c015ac208df429bf5ea6eeadffa87c346f1672aa885a6f65f99cfe` |
| callee entry `131C:0A5B` | `131C 0A5B 14A0 1D16 1D16 3D96 3DA6 0001 0005 7E40 0000 0001 3DA2 7246` | `39aac77ba469b8f399149aec5e1c1a2bcb7d84b32c232fefeff6513af77c17bb` |
| caller return `10D2:002F` | `10D2 002F 14A0 1D16 1D16 3D9A 3DA6 7E40 0005 0000 0005 0001 0000 7206` | `420da85f0c5ed4da09e42dbe7f32443cb5f5b6b7135d3bf24532ed740c3a62f9` |
| after `MOV CX,AX`, `10D2:0031` | `10D2 0031 14A0 1D16 1D16 3D9A 3DA6 7E40 0005 7E40 0005 0001 0000 7206` | same as `+002F` |

The call changes only SS:`3D96..3D99` in all 655,360 RAM bytes:
`A0 3D 16 1D` becomes `2F 00 D2 10`, the little-endian return
offset and caller segment. The arithmetic helper returns with
`AX=7E40`, `CX=0000`, `DX=0005`, and flags `7206`, and its execution
also changes four bytes at SS:`3D92..3D95` from `9E 3D 16 1D` to
`00 00 40 7E`. The `+002F..+0031` step changes only IP and CX;
its complete RAM image is byte-identical.

## Bounded C parity

`pr_rec_res_08ae_002a_call_multiply` writes the four return-frame
bytes at host-mapped SS:`[SP-4..SP-1]`, decrements SP, and enters the
relocated `0A5B` helper. `pr_rec_res_08ae_002f_copy_result` accepts
the **native observed return state** as a separate input and executes
only `MOV CX,AX`. It does not synthesize the helper's return values or
memory writes. The registered synthetic CTest checks the frame bytes,
segment relocation, 16-bit SP wrapping, and an alternate AX value.

The ignored native-seeded harness under
`resources/oracle-runs/slot-a-resident-137c-second-004/` compares
the full 640 KB RAM image and all 14 CPU words at both modeled
boundaries under `-std=c11 -Wall -Wextra -Wpedantic -Werror`:

| Bounded transition | RAM mismatches | CPU-word mismatches |
| --- | ---: | ---: |
| `+002A` to arithmetic entry | 0/655,360 | 0/14 |
| native `+002F` return to `+0031` | 0/655,360 | 0/14 |

The next instruction at `+0031` and the arithmetic helper's internal
behavior remain separate gates.

## Parity review

- **target_function:** reached `RES_08AE:+002A` far-call frame and
  `+002F` caller successor, each stopped at its next instruction.
- **source_candidates:** the two functions in
  `resident_time_caller_return_08ae_002a.c` cover only these two
  operations; the intervening arithmetic callee is excluded.
- **signals:** exact original and relocated call bytes; direct
  same-run stops at all four boundaries; full RAM and CPU equality
  at both modeled transitions; synthetic relocation, SP-wrap, and
  alternate-result checks.
- **score and verdict:** 4/5 corroborating signal groups, **match**
  for the two reached instruction boundaries. Independent second-run
  confirmation remains open.
- **review_notes:** the API requires its caller to map
  `return_frame` to SS:`[SP-4..SP-1]` and supply the relocated target
  segment. The native-seeded harness checks that mapping. The callee
  return CPU and RAM are observed inputs; no full-call parity is
  claimed.
