# Loaded RES_0AF8:0604 post-call decision boundary

Loaded `131C:0604`, physical `0x137C4`, is the continuation after the
indirect far call in the unique `131C:05FD` helper.
It maps to original image `0AF8:0604`, physical `0xB584`, and START
file `0x0B27A`. This audit treats the far callee's return as an
independently supplied frame. It does not assert that the call returned.

`deja "Poolrad 131C:011B parent join successor 131C:0604 post indirect call return side"`
found no indexed session. PM assigned this separate source-only gate
in inbox i5835; pF and p5 cleared ownership in i5847.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| Unique START helper | `0x0B273..0x0B283`, 17 bytes | `a0f2252dd20ab5c8e6209a7a334f97d893ad1ac671d8ceb2fb825674c1b2e4b1` |
| Unique return suffix | START `0x0B27A..0x0B283`, loaded `0x137C4..0x137CD`, 10 bytes | `89ccf1ac7c0aad9c03d35681e115f8b2953d425add72c23a830b90f3f0a2043b` |
| First decision | START `0x0B27A..0x0B27D`, loaded `0x137C4..0x137C7`, 4 bytes | `66faa6ae049f7aa3904180632d00f46e3abd7aad7db42a98a4ec6c023c677858` |
| Nonzero successor | START `0x0B27E..0x0B280`, loaded `0x137C8..0x137CA`, 3 bytes | `bf754db91e10c283bf2b78bd69d4d97ccf871861a819899ad2648cdd9ba5722e` |

The ten-byte suffix matches loaded RAM and occurs once in START. The
first two instructions, `OR AX,AX` and `JE`, set ZF from **the AX
returned by the external far callee** and branch to `131C:060B`
when AX is zero. A nonzero AX falls through to a word store at
`DS:4426`; the zero path skips that store. The source then has
`POP DI`, `POP ES`, and `RET` on both paths. The four-byte decision
alone has three START occurrences, so the unique helper and suffix
anchor its identity.

An actual continuation needs post-call AX and the complete DS and
SS:SP memory frame. The far callee may have changed those values and
may have consumed stack arguments. Neither its target nor cleanup
convention is proven here. This gate stops before claiming the
`DS:4426` store, pops, or return as an observed effect. Original and
loaded physical addresses differ by `0x08240`; original and START
file addresses differ by `0x030A`.

Reproduce with
`python3 tools/replay/slot_a_res0af8_0604_return_decision_audit.py`.
No native reach, C parity, or full-C count is claimed. Original media
and RAM stay ignored under `resources/`.
