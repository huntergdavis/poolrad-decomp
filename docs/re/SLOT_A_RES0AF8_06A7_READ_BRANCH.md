# Loaded RES_0AF8:06A7 read-branch source audit

The [merged `+0660` caller arm](SLOT_A_RES0AF8_0660_PREDECESSOR_ARM.md)
can call `+0690`, whose prefix joins loaded `131C:06A7`.
This source-only audit covers `+06A7..+06C2` and stops
before `+06C3`. It does not establish that the call or either
DOS operation ran in the saved-slot replay.

`deja "Poolrad RES_0AF8 131C:0690 near callee 06A1 06A7 source boundaries"`
found no indexed match. PM assigned the two disjoint screens in
inbox i6404. p5 and pF cleared them; pC's no-overlap reply was
relayed by PM in inbox i6432. Original `0AF8:06A7` is physical
`0xB627`, START source `0x0B31D`, and loaded physical `0x13867`.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START span | `0x0B31D..0x0B338`, 28 bytes | `99ae350d67e58e7c14024fa77cbde4d5e5ce1289d2b34bc5103bc7caa5dc0b02` |
| Loaded span | `0x13867..0x13882`, 28 bytes | `99ae350d67e58e7c14024fa77cbde4d5e5ce1289d2b34bc5103bc7caa5dc0b02` |
| JAE | START `0x0B335..0x0B336`, loaded `0x1387F..0x13880`, 2 bytes | `0105a3904ec8a03179d0ac37e167c66f513b69a99429cd8311826a5aba05a220` |

The unique span has twelve instructions. It copies DX to CX,
AX to DX, loads BX from the then-current `DS:[DI]`, sets
AX=`0x4200`, and calls `INT 21h`. It then forms DX as
`DI+0x80`, sets CX=`0x80`, reloads BX from `DS:[DI]`,
sets AH=`0x3F`, and calls `INT 21h` again. A clear carry
flag after the second interrupt takes `JAE +06C3`; a set
carry flag falls through to `+06C1`, clears AX, and joins
`+06C3`. Both interrupt results, supplied `DS:DI`, actual
path, native reach, and later effects are unmeasured.

The original span occurs once in START and matches loaded RAM.
Original and loaded physical addresses differ by `0x08240`;
original and START file addresses differ by `0x030A`. Reproduce with
`python3 tools/replay/slot_a_res0af8_06a7_read_branch_audit.py`.
No C parity or full-C count is claimed. Original media and RAM stay
ignored under `resources/`.
