# Loaded RES_0AF8:0676 frame-store source audit

The [merged `+063F` JE](SLOT_A_RES0AF8_063F_TYPE_DECISION.md)
takes loaded `131C:0676` when a supplied word at
`DS:[DI+2]` equals `0xD7B1`. Other predecessor routes may
also join there; this audit does not establish their runtime
outcomes. The source-only span ends before shared `+068C`,
which begins with a DS pop and far return.

`deja "Poolrad RES_0AF8 131C:064D 131C:0676 JE successors source audit"`
timed out after 15 seconds, so no indexed result was reused.
PM assigned the disjoint address in inbox i6262; p5, pF, and
pC explicitly cleared both addresses in PM inbox i6279. Original
`0AF8:0676` is physical `0xB5F6`, START source `0x0B2EC`,
and loaded physical `0x13836`.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START span | `0x0B2EC..0x0B301`, 22 bytes | `af01f1858dfabab8997d4182ac0c7b3f8c97c832e97a6bd41dcc3521342b2289` |
| Loaded span | `0x13836..0x1384B`, 22 bytes | `af01f1858dfabab8997d4182ac0c7b3f8c97c832e97a6bd41dcc3521342b2289` |
| Next shared return | START `0x0B302..0x0B305`, loaded `0x1384C..0x1384F`, 4 bytes | `45755c99e39cfb753aa25ef66894dda3ef31d15961c9ce3ebf533bd8fb4145f1` |

The unique span has seven instructions. It stores incoming
AX at `DS:[DI+0x14]`, CS at `+0x16`, incoming CX at `+0x18`,
incoming BX at `+0x1A`, `0x075F` at `+0x1C`, and CS at
`+0x1E`, all as words in that order. It then clears AX.
The following shared `+068C` epilogue is pinned only as a
boundary; it is outside this span. The supplied `DS:DI`,
register values, actual stores, native reach, and path are
unmeasured.

The original span occurs once in START and matches loaded RAM.
Original and loaded physical addresses differ by `0x08240`;
original and START file addresses differ by `0x030A`. Reproduce with
`python3 tools/replay/slot_a_res0af8_0676_frame_store_audit.py`.
No C parity or full-C count is claimed. Original media and RAM stay
ignored under `resources/`.
