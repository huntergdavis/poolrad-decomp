# Loaded RES_0AF8:0660 predecessor-arm source audit

The [merged `+064D` JNE](SLOT_A_RES0AF8_064D_INT_DECISION.md)
falls through to loaded `131C:0660` when the tested DL bit is
clear. This source-only arm ends before the [merged `+0676`
frame stores](SLOT_A_RES0AF8_0676_FRAME_STORE.md). Its near
callee at `+0690` is outside this audit.

`deja "Poolrad RES_0AF8 131C:068C shared epilogue 0660 0671 predecessor arm source"`
found no indexed match. PM assigned this screen in inbox i6324;
pF i6332, p5 i6333, and pC i6339 explicitly cleared both
disjoint slices. Original `0AF8:0660` is physical `0xB5E0`,
START source `0x0B2D6`, and loaded physical `0x13820`.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START span | `0x0B2D6..0x0B2EB`, 22 bytes | `7b5b1c35a0e1b0bfb687fd426aec8f2bd67ae0cbfb795ffcbf884db880d1d749` |
| Loaded span | `0x13820..0x13835`, 22 bytes | `7b5b1c35a0e1b0bfb687fd426aec8f2bd67ae0cbfb795ffcbf884db880d1d749` |
| Near call | START `0x0B2DD..0x0B2DF`, loaded `0x13827..0x13829`, 3 bytes | `f4cee570c6b2dd23d00ed873768e45ed3b24f03ceeee06fb133ef5559ba8c073` |

The unique span has seven instructions. It compares word
`DS:[DI+2]` with `0xD7B3`; inequality jumps over the near
call to `+066A`, while equality calls `+0690` and, if that
callee returns, continues at `+066A`. The shared continuation
sets AX=`0x071A`, clears CX and BX, and stores word
`0xD7B2` at the then-current `DS:[DI+2]` before joining
`+0676`. The callee's side effects and return are unresolved,
so the supplied `DS:DI`, actual write, native reach, and path
remain unmeasured. No bytes from the merged `+0676` span are
included.

The original span occurs once in START and matches loaded RAM.
Original and loaded physical addresses differ by `0x08240`;
original and START file addresses differ by `0x030A`. Reproduce with
`python3 tools/replay/slot_a_res0af8_0660_predecessor_arm_audit.py`.
No C parity or full-C count is claimed. Original media and RAM stay
ignored under `resources/`.
