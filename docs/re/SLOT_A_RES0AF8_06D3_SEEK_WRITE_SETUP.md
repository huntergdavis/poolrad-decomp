# Loaded RES_0AF8:06D3 seek/write setup source audit

The [merged `+06C9` scan arm](SLOT_A_RES0AF8_06C9_SCAN_LOOP.md)
branches to loaded `131C:06D3` on its byte comparison.
This source-only audit covers `+06D3..+06E8` and stops before
the separate `+06E9` return. It does not establish that the
branch or either DOS interrupt ran in the saved-slot replay.

`deja "Poolrad RES_0AF8 131C:06D3 06E9 tail DOS write near return source"`
found no indexed match. PM cleared ownership of both later
spans in inbox i6546, after p5, pF, and pC checks. Original
`0AF8:06D3` is physical `0xB653`, START source `0x0B349`,
and loaded physical `0x13893`.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START span | `0x0B349..0x0B35E`, 22 bytes | `0c1874990854d17cc19562456b2816d737443d20da7281b24b990ee3fe747ad7` |
| Loaded span | `0x13893..0x138A8`, 22 bytes | `0c1874990854d17cc19562456b2816d737443d20da7281b24b990ee3fe747ad7` |

The ten instructions copy BX to DX, subtract AX from DX,
set CX to `0xFFFF`, load BX from `DS:[DI]`, set AX to
`0x4202`, and invoke `INT 21h`. They then clear CX, reload
BX from `DS:[DI]`, set AH to `0x40`, and invoke `INT 21h`
again. This is the source order of the DOS seek and zero-count
write requests. The live incoming AX and BX, `DS:[DI]` value,
interrupt results, actual path, native reach, and later effects
are unmeasured.

The 22-byte span occurs once in START and matches loaded RAM.
Original and loaded physical addresses differ by `0x08240`;
original and START file addresses differ by `0x030A`. Reproduce
with `python3 tools/replay/slot_a_res0af8_06d3_seek_write_setup_audit.py`.
No C parity or full-C count is claimed. Original media and RAM
stay ignored under `resources/`.
