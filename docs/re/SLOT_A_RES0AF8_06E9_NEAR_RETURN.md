# Loaded RES_0AF8:06E9 near-return source audit

The [merged `+06C3` branch](SLOT_A_RES0AF8_06C3_SCAN_START.md)
can jump to loaded `131C:06E9`; the separately audited
`+06D3..+06E8` source span also ends here. This source-only
audit covers the single instruction at `+06E9` and does not
establish native reach or a caller return frame.

`deja "Poolrad RES_0AF8 131C:06D3 06E9 tail DOS write near return source"`
found no indexed match. PM cleared ownership of both later
spans in inbox i6546 after p5, pF, and pC checks. Original
`0AF8:06E9` is physical `0xB669`, START source `0x0B35F`,
and loaded physical `0x138A9`.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START return | `0x0B35F`, 1 byte | `ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e` |
| Loaded return | `0x138A9`, 1 byte | `ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e` |
| START context | `0x0B349..0x0B35F`, 23 bytes | `6c68916d956424a3c1295f94615cda763edb04fbf32a0d6cc4fb6124efbb8ef6` |
| Loaded context | `0x13893..0x138A9`, 23 bytes | `6c68916d956424a3c1295f94615cda763edb04fbf32a0d6cc4fb6124efbb8ef6` |

The instruction is a near `RET`. The one-byte source occurs
140 times in START, so the preceding 23-byte context anchors
this return uniquely and matches loaded RAM. The context is
for identity only; this audit does not re-own the preceding
22-byte body. The live path, stack, and caller are unmeasured.

Original and loaded physical addresses differ by `0x08240`;
original and START file addresses differ by `0x030A`. Reproduce
with `python3 tools/replay/slot_a_res0af8_06e9_return_audit.py`.
No C parity or full-C count is claimed. Original media and RAM
stay ignored under `resources/`.
