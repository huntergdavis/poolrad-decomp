# Loaded RES_0AF8:0615 first word-store boundary

The preceding prologue at loaded `131C:060E..0614` loads DS:DI
from an SS caller frame. This source-only audit bounds the next four
bytes, through the first word write at `131C:0617`. It stops before
the `MOV AX,0x3D00` at `131C:0619`.

`deja "Poolrad RES_0AF8 131C:0149 callee return boundary 131C:0615 XOR first write source"`
found no indexed session. PM assigned this disjoint follow-on in
inbox i6025 and reported p5/pF ownership clearance. Original
`0AF8:0615` is physical `0xB595`; START file source is
`0x0B28B`; loaded physical is `0x137D5`.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START zero-store span | `0x0B28B..0x0B28E`, 4 bytes | `7234769f818ccc0d93e92db72ded096dd584f1584c72eacc560f804ae64c8edb` |
| Loaded zero-store span | `0x137D5..0x137D8`, 4 bytes | `7234769f818ccc0d93e92db72ded096dd584f1584c72eacc560f804ae64c8edb` |
| Write instruction | START `0x0B28D..0x0B28E`, loaded `0x137D7..0x137D8`, 2 bytes | `9789c11eed3409fb170ed9c09f5bb697149679344202c0653f1e06609201b454` |
| Prologue plus store anchor | START `0x0B284..0x0B28E`, loaded `0x137CE..0x137D8`, 11 bytes | `12586dd6325cca0490499dcc572c305f1f6d646052d506fe37e7f68dd8c2ae8c` |

The four-byte span occurs once in START and matches loaded RAM. It
decodes to `XOR CX,CX`, then `MOV [DI],CX`. If execution reaches
this span with a writable DS:DI word, CX becomes zero and the word
at DS:DI becomes zero. The XOR changes FLAGS; the MOV does not.
The prior `LDS` would obtain DS:DI from the SS frame at entry
SP+`4` and SP+`6`, but that frame and pointer have not been
measured. This audit does not establish an actual write or its
address in native execution. Original and loaded physical addresses
differ by `0x08240`; original and START file addresses differ by
`0x030A`.

Reproduce with
`python3 tools/replay/slot_a_res0af8_0615_zero_store_audit.py`.
No native reach, C parity, or full-C count is claimed. Original media
and RAM stay ignored under `resources/`.
