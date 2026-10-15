# Loaded RES_0AF8:064D interrupt decision source audit

The [merged `+063F` JE](SLOT_A_RES0AF8_063F_TYPE_DECISION.md)
falls through to loaded `131C:064D` when a supplied word at
`DS:[DI+2]` differs from `0xD7B1`. This source-only audit ends
at the JNE `+065E`, before either successor.

`deja "Poolrad RES_0AF8 131C:064D 131C:0676 JE successors source audit"`
timed out after 15 seconds, so no indexed result was reused.
PM assigned the disjoint address in inbox i6262; p5, pF, and
pC explicitly cleared both addresses in PM inbox i6279. Original
`0AF8:064D` is physical `0xB5CD`, START source `0x0B2C3`,
and loaded physical `0x1380D`.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START span | `0x0B2C3..0x0B2D5`, 19 bytes | `af608b02be4e02d7d07efe62a86600758e69bba2df4787400d3a7339f6864e82` |
| Loaded span | `0x1380D..0x1381F`, 19 bytes | `af608b02be4e02d7d07efe62a86600758e69bba2df4787400d3a7339f6864e82` |
| JNE | START `0x0B2D4..0x0B2D5`, loaded `0x1381E..0x1381F`, 2 bytes | `33ce820402bdf76bc8ab0bdb085802782e50d60a6f2d2d3956663b516d222924` |

The unique span has eight instructions. It loads BX from
`DS:[DI]`, sets AX=`0x4400`, calls `INT 21h`, then sets
AX=`0x073F`, CX=AX, and BX=CS. It tests bit 7 of DL and
branches to `+0671` when set; otherwise it falls through
to `+0660`. The interrupt may change registers, flags, and
external state. The supplied `DS:DI`, interrupt effect,
DL value, native reach, and actual branch are unmeasured.

The original span occurs once in START and matches loaded RAM.
Original and loaded physical addresses differ by `0x08240`;
original and START file addresses differ by `0x030A`. Reproduce with
`python3 tools/replay/slot_a_res0af8_064d_int_decision_audit.py`.
No C parity or full-C count is claimed. Original media and RAM stay
ignored under `resources/`.
