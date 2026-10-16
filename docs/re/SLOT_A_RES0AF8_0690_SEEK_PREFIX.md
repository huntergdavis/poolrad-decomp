# Loaded RES_0AF8:0690 near-callee prefix source audit

The [merged `+0660` predecessor arm](SLOT_A_RES0AF8_0660_PREDECESSOR_ARM.md)
contains a conditional near call to loaded `131C:0690`. This
source-only audit covers `+0690..+06A6` and stops before its
join at `+06A7`; it does not claim that the call ran in the
saved-slot replay.

`deja "Poolrad RES_0AF8 131C:0690 near callee 06A1 06A7 source boundaries"`
found no indexed match. PM assigned the two disjoint screens in
inbox i6404. p5 and pF cleared them; pC's no-overlap reply was
relayed by PM in inbox i6432. Original `0AF8:0690` is physical
`0xB610`, START source `0x0B306`, and loaded physical `0x13850`.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START span | `0x0B306..0x0B31C`, 23 bytes | `da932359fa8a1900f9b3bb57f8b2d1425da8c9a1fdb55f0155a728f6845f57b5` |
| Loaded span | `0x13850..0x13866`, 23 bytes | `da932359fa8a1900f9b3bb57f8b2d1425da8c9a1fdb55f0155a728f6845f57b5` |
| JAE | START `0x0B317..0x0B318`, loaded `0x13861..0x13862`, 2 bytes | `a16acfba5d54a1f2edb21cabe3f51fbc5b61a01f265791c48affc9a9476d1601` |

The unique span has ten instructions. It clears DX and CX,
loads BX from `DS:[DI]`, sets AX=`0x4202`, and executes
`INT 21h`. It then subtracts `0x80` from AX, borrows into
DX, and uses the resulting unsigned carry condition for
`JAE +06A7`. If the branch is not taken, it clears AX and
DX before the same `+06A7` join. The DOS interrupt result,
supplied `DS:DI`, actual branch, native reach, and later
effects are unmeasured.

The original span occurs once in START and matches loaded RAM.
Original and loaded physical addresses differ by `0x08240`;
original and START file addresses differ by `0x030A`. Reproduce with
`python3 tools/replay/slot_a_res0af8_0690_seek_prefix_audit.py`.
No C parity or full-C count is claimed. Original media and RAM stay
ignored under `resources/`.
