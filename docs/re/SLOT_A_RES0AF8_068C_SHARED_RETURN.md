# Loaded RES_0AF8:068C shared return source audit

The [merged `+0676` store span](SLOT_A_RES0AF8_0676_FRAME_STORE.md)
falls through to loaded `131C:068C`. The [merged `+0636`
carry branch](SLOT_A_RES0AF8_0636_DOS_BRANCH.md) also targets
`+068C`. This source-only audit covers just `+068C..+068F`,
the shared epilogue. Neither predecessor is claimed as a
measured native route.

`deja "Poolrad RES_0AF8 131C:068C shared epilogue 0660 0671 predecessor arm source"`
found no indexed match. PM assigned this screen in inbox i6324;
pF i6332, p5 i6333, and pC i6339 explicitly cleared both
disjoint slices. Original `0AF8:068C` is physical `0xB60C`,
START source `0x0B302`, and loaded physical `0x1384C`.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| Return START span | `0x0B302..0x0B305`, 4 bytes | `45755c99e39cfb753aa25ef66894dda3ef31d15961c9ce3ebf533bd8fb4145f1` |
| Return loaded span | `0x1384C..0x1384F`, 4 bytes | `45755c99e39cfb753aa25ef66894dda3ef31d15961c9ce3ebf533bd8fb4145f1` |
| Context START span | `0x0B2EC..0x0B305`, 26 bytes | `246f207460c20748409b24362da282047f90d7d76951a98e1d8963c3271b0715` |
| Context loaded span | `0x13836..0x1384F`, 26 bytes | `246f207460c20748409b24362da282047f90d7d76951a98e1d8963c3271b0715` |

The four-byte return occurs six times in START, so its raw
sequence alone does not identify this location. The 26-byte
context from the merged `+0676` span through this return
occurs once and matches loaded RAM. The earlier 22 bytes are
an address anchor only; their ownership remains with the
merged `+0676` audit.

The two epilogue instructions are `POP DS` and `RETF 4`.
Statically, they restore DS from the stack and perform a far
return that discards four argument bytes. The supplied stack,
return destination, native reach, and path are unmeasured.
Original and loaded physical addresses differ by `0x08240`;
original and START file addresses differ by `0x030A`.

Reproduce with
`python3 tools/replay/slot_a_res0af8_068c_shared_return_audit.py`.
No C parity or full-C count is claimed. Original media and RAM stay
ignored under `resources/`.
