# Second-key resident caller `+00C4` comparison gate

This bounded gate starts at the directly reached `10D2:00C4` stop,
loads two caller-local WORDs, compares the high WORDs and then the low
WORDs if needed, and stops before the reached `10D2:0089` `PUSH CS`.
The following near `CALL` into the resident `+0000` callee is outside
this gate. The other successor, `+00D6` epilogue, is not reached by
this native input. No full-function or complete second-key credit is
claimed.

## Source and native stops

The required `deja "Poolrad RES_08AE +00C4 second key conditional successor native parity"`
query found no indexed prior session. The merged
[`+0036` same-run note](SLOT_A_SECOND_KEY_RESIDENT_0036_MUL.md)
was reused for input provenance and full-state replay discipline;
the `+00C4` and `+0089` stops below were measured directly.

The controlled slot-A run used DOS v1.3 START.EXE SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and 13,137-byte slot-A SAVGAMA.DAT SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
Original START file `0x8BBB..0x8BCC` and loaded `10D2:00C4..00D5`
both contain `8B 46 F8 8B 56 FA 3B 56 FE 7F 07 7C B8 3B 46 FC 72 B3`,
SHA-256 `87f8e49c385cdecc7689dba139dbbe175db242d399eb70b3bc6dc559ae10f80a`.

At entry BP=`3DB4h`, so SS:`[BP-8..BP-1]` contains little-endian
WORDs `8D94 0005 8D9E 0005`. `MOV AX,[BP-8]` and `MOV DX,[BP-6]`
produce `DX:AX=0005:8D94`. The high comparison with `[BP-2]=0005`
is equal; `JG` and `JL` fall through. The low comparison with
`[BP-4]=8D9E` sets CF, so `JB` returns to `+0089` before the next
callee invocation.

The 14 CPU words are ordered `CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS`:

| Stop | CPU words | 640 KB RAM SHA-256 |
| --- | --- | --- |
| `10D2:00C4` | `10D2 00C4 14A0 1D16 1D16 3DAC 3DB4 000A 0000 000A 0000 0001 0000 7246` | `420da85f0c5ed4da09e42dbe7f32443cb5f5b6b7135d3bf24532ed740c3a62f9` |
| `10D2:0089` | `10D2 0089 14A0 1D16 1D16 3DAC 3DB4 8D94 0000 000A 0005 0001 0000 7297` | same |

Direct entry and exit RAM dumps are byte identical.

## Bounded C parity

`pr_rec_res_08ae_00c4_branch` reads the eight caller-local bytes,
updates AX/DX and materialized arithmetic FLAGS, and selects `+0089`
or `+00D6`. It does not mutate RAM or execute either successor.
The ignored strict-C11 native-seeded harness compares **0/655,360
RAM byte and 0/14 CPU-word mismatches**. The registered focused CTest
covers the reached low-WORD borrow, high-WORD signed greater and less,
low-WORD equality and greater, and signed-overflow comparison cases.
Synthetic cases check instruction behavior without adding native path
claims. Private captures remain in ignored `resources/`.

## Parity review

- **target_function:** reached `RES_08AE:+00C4..+00D5`, stopping
  before the nested-call setup at `+0089`.
- **source_candidates:** `pr_rec_res_08ae_00c4_branch` models the
  local loads, high/low comparisons, and conditional successors.
- **signals:** exact original and loaded bytes; direct same-run entry
  and exit CPU/RAM; full-state C replay; focused synthetic branch tests.
- **score and verdict:** 4/5 corroborating signal groups, **match** for
  the reached low-WORD borrow route. Other native comparison outcomes
  remain unobserved.
- **review_notes:** the `+0089 PUSH CS; +008A CALL` and the `+00D6`
  epilogue are outside this gate. The caller-local pointer must map
  SS:`[BP-8]` at entry. This is bounded instruction parity with zero
  strict-function and playable-game credit.
