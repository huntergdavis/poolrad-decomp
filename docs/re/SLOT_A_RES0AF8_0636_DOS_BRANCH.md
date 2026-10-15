# Loaded RES_0AF8:0636 DOS branch source audit

The [merged second JE](SLOT_A_RES0AF8_0623_0630_SUCCESSORS.md)
falls through to loaded `131C:0636` when the supplied byte at
`DS:[DI+0x30]` is nonzero. This source-only audit ends before
`131C:063F`; the carry branch targets `131C:068C`.

`deja "Poolrad RES_0AF8 131C:0636 131C:063F second JE successor source audit"`
timed out after 15 seconds, so no indexed result was reused.
PM assigned this address in inbox i6192. p5, pF, and pC received
exact-address ownership screens before edits. Original `0AF8:0636`
is physical `0xB5B6`, START source `0x0B2AC`, and loaded physical
`0x137F6`.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START span | `0x0B2AC..0x0B2B4`, 9 bytes | `e3b89529f92d647af55495aff2aa884615aab6b031529082362eec35885dd158` |
| Loaded span | `0x137F6..0x137FE`, 9 bytes | `e3b89529f92d647af55495aff2aa884615aab6b031529082362eec35885dd158` |
| Carry branch | START `0x0B2B1..0x0B2B2`, loaded `0x137FB..0x137FC`, 2 bytes | `4a35e73f0b860ba321735b7350646cb0c126f98ee7386eef3bcba5fdc7ea17f5` |

The unique span has four instructions: `LEA DX,[DI+0x30]`,
`INT 21h`, `JC +068C`, and `MOV DS:[DI],AX`. The interrupt can
change registers, flags, and external state. A set carry flag takes
`+068C`; a clear carry flag executes the word store and then joins
`+063F`. Incoming `AH`, `DS:DI`, interrupt result, actual store,
native reach, and branch outcome are unmeasured. In particular,
the interrupt function is not inferred from a different predecessor.

The original span occurs once in START and matches loaded RAM.
Original and loaded physical addresses differ by `0x08240`;
original and START file addresses differ by `0x030A`. Reproduce with
`python3 tools/replay/slot_a_res0af8_0636_dos_branch_audit.py`.
No C parity or full-C count is claimed. Original media and RAM stay
ignored under `resources/`.
