# Loaded RES_0AF8:06C3 scan-start source audit

The [merged `+06A7` read branch](SLOT_A_RES0AF8_06A7_READ_BRANCH.md)
joins loaded `131C:06C3`. This source-only audit covers
`+06C3..+06C8`, stopping before `+06C9`; it does not establish
native reach or the taken branch in the saved-slot replay.

`deja "Poolrad RES_0AF8 131C:06C3 06C9 scan loop successors source"`
found no indexed match. PM cleared ownership of `+06C3..+06D2`
in inbox i6491 after p5, pF, and pC checks. Original
`0AF8:06C3` is physical `0xB643`, START source `0x0B339`, and
loaded physical `0x13883`.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START span | `0x0B339..0x0B33E`, 6 bytes | `f8aa5307503265eb9d552a2ec4bef725fa26ac5a516b1611d5c8bbe2db82d1e8` |
| Loaded span | `0x13883..0x13888`, 6 bytes | `f8aa5307503265eb9d552a2ec4bef725fa26ac5a516b1611d5c8bbe2db82d1e8` |

The three instructions clear BX, compare it with AX, then take
`JE +06E9` when AX is zero. Otherwise execution falls through
to `+06C9`. The value of AX, native reach, selected path, and
later effects are unmeasured.

The six-byte span occurs once in START and matches loaded RAM.
Original and loaded physical addresses differ by `0x08240`;
original and START file addresses differ by `0x030A`. Reproduce
with `python3 tools/replay/slot_a_res0af8_06c3_scan_start_audit.py`.
No C parity or full-C count is claimed. Original media and RAM
stay ignored under `resources/`.
