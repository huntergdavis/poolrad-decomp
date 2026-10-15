# Loaded RES_0AF8:0623 and 0630 source successors

The [merged first decision](SLOT_A_RES0AF8_0619_FIRST_JE.md)
falls through to loaded `131C:0623` when the `+0621` JE is not
taken. Its taken target is `131C:0630`. These two source-only
spans are audited independently. The first ends at its JE
`+062C`; the second ends at its JE `+0634`. The intervening
`+062E` instruction is pinned as a boundary, with its effect
outside this audit.

`deja "Poolrad 131C:0155 MOV SI 01EE 131C:0623 0630 source branch audit"`
found no indexed session. PM assigned the disjoint successor gate
in inbox i6155. p5, pF, and pC received exact-address ownership
screens before edits. Original `0AF8:0623` is physical `0xB5A3`,
START source `0x0B299`, loaded physical `0x137E3`. Original
`0AF8:0630` is physical `0xB5B0`, START source `0x0B2A6`,
loaded physical `0x137F0`.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START `+0623` span | `0x0B299..0x0B2A3`, 11 bytes | `96fd30f7a96321cb7b3dbad93350291f05eb5e14e8a2d88d99e45ee824856584` |
| Loaded `+0623` span | `0x137E3..0x137ED`, 11 bytes | `96fd30f7a96321cb7b3dbad93350291f05eb5e14e8a2d88d99e45ee824856584` |
| First JE | START `0x0B2A2..0x0B2A3`, loaded `0x137EC..0x137ED`, 2 bytes | `0b358362552792220751c58c4cca9c4e54407d2339649a0b96182880f0504c1a` |
| Intervening instruction | START `0x0B2A4..0x0B2A5`, loaded `0x137EE..0x137EF`, 2 bytes | `0b83e1d7c4f0763abe7a196d07183643d6247f3a7b23dc59999abe7d81e4875e` |
| START `+0630` span | `0x0B2A6..0x0B2AB`, 6 bytes | `fb657a460f00ab1a148c477ec43c70204228d6d97ca8723fcec965d5992bb56e` |
| Loaded `+0630` span | `0x137F0..0x137F5`, 6 bytes | `fb657a460f00ab1a148c477ec43c70204228d6d97ca8723fcec965d5992bb56e` |
| Second JE | START `0x0B2AA..0x0B2AB`, loaded `0x137F4..0x137F5`, 2 bytes | `e6332f7e34686e19d5ad393559009f37d77126d1221407bfdee68bec60234a29` |

Each main span occurs once in START and matches loaded RAM. The
`+0623` span decodes to `MOV AL,2`, `INC word DS:[DI]`,
`CMP word DS:[DI+2],0xD7B3`, and `JE +0630`.
Given a writable supplied DS:DI, the INC increments that word
modulo 65536. If the later comparison is equal, this JE takes
`+0630`; otherwise it falls through to `+062E`. The
`+062E` instruction decodes to `MOV AH,0x3C`, but its effect
is not included here.

The independent `+0630` span decodes to
`CMP byte DS:[DI+0x30],0` and `JE +063F`. A zero byte
takes `+063F`; a nonzero byte falls through to `+0636`.
The supplied pointer and memory values, actual writes, native
branch outcomes, and successor effects are unmeasured. Original
and loaded physical addresses differ by `0x08240`; original
and START file addresses differ by `0x030A`.

Reproduce with
`python3 tools/replay/slot_a_res0af8_0623_0630_successors_audit.py`.
No native reach, C parity, or full-C count is claimed. Original
media and RAM stay ignored under `resources/`.
