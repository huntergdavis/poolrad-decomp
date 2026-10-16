# Loaded RES_0AF8:06C9 scan-loop source audit

The [merged `+06A7` read branch](SLOT_A_RES0AF8_06A7_READ_BRANCH.md)
joins loaded `131C:06C3`; the separately audited `+06C3` branch
can fall through to `+06C9`. This source-only audit covers
`+06C9..+06D2`, stopping before `+06D3`; it does not establish
native reach or the selected path in the saved-slot replay.

`deja "Poolrad RES_0AF8 131C:06C3 06C9 scan loop successors source"`
found no indexed match. PM cleared ownership of `+06C3..+06D2`
in inbox i6491 after p5, pF, and pC checks. Original
`0AF8:06C9` is physical `0xB649`, START source `0x0B33F`, and
loaded physical `0x13889`.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START span | `0x0B33F..0x0B348`, 10 bytes | `4d4d9bd5f1aea8603bbf40a0846d5c3af27da0a9af337196c7587df371a9ec17` |
| Loaded span | `0x13889..0x13892`, 10 bytes | `4d4d9bd5f1aea8603bbf40a0846d5c3af27da0a9af337196c7587df371a9ec17` |

The four instructions compare byte `DS:[BX+DI+0x80]` with
`0x1A`, take `JE +06D3` on equality, or increment BX and jump
back to `+06C5` for the next comparison with AX. The buffer,
AX, actual branch, native reach, and later effects are unmeasured.

The ten-byte span occurs once in START and matches loaded RAM.
Original and loaded physical addresses differ by `0x08240`;
original and START file addresses differ by `0x030A`. Reproduce
with `python3 tools/replay/slot_a_res0af8_06c9_scan_loop_audit.py`.
No C parity or full-C count is claimed. Original media and RAM
stay ignored under `resources/`.
