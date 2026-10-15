# Loaded RES_0AF8:063F type decision source audit

The [merged second JE](SLOT_A_RES0AF8_0623_0630_SUCCESSORS.md)
takes loaded `131C:063F` when the supplied byte at
`DS:[DI+0x30]` is zero. The separate `+0636` path may also
join here after its DOS interrupt and clear-carry store; this
audit does not establish that outcome. This source-only span
ends at its JE `+064B`.

`deja "Poolrad RES_0AF8 131C:0636 131C:063F second JE successor source audit"`
timed out after 15 seconds, so no indexed result was reused.
PM assigned this address in inbox i6192. p5, pF, and pC received
exact-address ownership screens before edits. Original `0AF8:063F`
is physical `0xB5BF`, START source `0x0B2B5`, and loaded physical
`0x137FF`.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START span | `0x0B2B5..0x0B2C2`, 14 bytes | `09644656af005a31725db0d39af0e866be844ceb62fbadac6809def3a4879097` |
| Loaded span | `0x137FF..0x1380C`, 14 bytes | `09644656af005a31725db0d39af0e866be844ceb62fbadac6809def3a4879097` |
| JE | START `0x0B2C1..0x0B2C2`, loaded `0x1380B..0x1380C`, 2 bytes | `49f25187c36d64f88fdd63a8b0d1bc3c8aabcb4c0e3bd565efa1bea5fc6e8a6a` |

The unique span has five instructions: `MOV AX,0x06EA`,
`XOR CX,CX`, `MOV BX,CX`, `CMP word DS:[DI+2],0xD7B1`,
and `JE +0676`. The register operations leave AX=`0x06EA`,
CX=0, and BX=0. If the supplied word is `0xD7B1`, the JE
takes `+0676`; otherwise it falls through to `+064D`.
No memory write occurs inside this span. The supplied `DS:DI`,
word value, native reach, branch outcome, and later effects
are unmeasured.

The original span occurs once in START and matches loaded RAM.
Original and loaded physical addresses differ by `0x08240`;
original and START file addresses differ by `0x030A`. Reproduce with
`python3 tools/replay/slot_a_res0af8_063f_type_decision_audit.py`.
No C parity or full-C count is claimed. Original media and RAM stay
ignored under `resources/`.
