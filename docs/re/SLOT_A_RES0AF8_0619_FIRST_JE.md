# Loaded RES_0AF8:0619 first post-store decision

The prior `131C:0615..0618` span zeros a word through a supplied
DS:DI pointer if reached. This source-only audit bounds the next ten
bytes at `131C:0619..0622`, through the first `JE` at `+0621`.
It stops before either successor: fallthrough `+0623` and taken
target `+0630`.

`deja "Poolrad RES_0AF8 loaded 131C:0152 next caller instruction 131C:0619 MOV AX 3D00 control boundary"`
found no indexed session. PM assigned this disjoint follow-on in
inbox i6083; p5, pF, and pC were screened for ownership before edits.
Original `0AF8:0619` is physical `0xB599`; START file source
is `0x0B28F`; loaded physical is `0x137D9`.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START decision | `0x0B28F..0x0B298`, 10 bytes | `1dd96e233790b8fdbac81475a618a39b2f5f639e855bc5b6bce2a80b9df15b82` |
| Loaded decision | `0x137D9..0x137E2`, 10 bytes | `1dd96e233790b8fdbac81475a618a39b2f5f639e855bc5b6bce2a80b9df15b82` |
| JE | START `0x0B297..0x0B298`, loaded `0x137E1..0x137E2`, 2 bytes | `b2a2028749af082a496044f7e1e70ceb4d21b57aedc24eacb85fc7373caa7b04` |
| Taken target instruction | START `0x0B2A6..0x0B2A9`, loaded `0x137F0..0x137F3`, 4 bytes | `9585a4fad7065049202d052c78cd9efc9e59b068e8fada1c3d4c7b6a2d925560` |
| Fallthrough instruction | START `0x0B299..0x0B29A`, loaded `0x137E3..0x137E4`, 2 bytes | `944e63bf90ea3bdaf95ae2c35787a51f132205f9e8240d635413a3aa2c3026a9` |

The ten-byte decision occurs once in START and matches loaded RAM.
It decodes to `MOV AX,0x3D00`, `CMP word DS:[DI+2],0xD7B1`,
then `JE +0630`. If the supplied word equals `0xD7B1`, the
branch targets `+0630`; otherwise execution falls through to
`+0623`. The target and fallthrough begin at verified instruction
boundaries, but their effects are outside this gate. The actual
DS:DI caller frame, comparison value, and native branch outcome
are unmeasured. Original and loaded physical addresses differ by
`0x08240`; original and START file addresses differ by `0x030A`.

Reproduce with
`python3 tools/replay/slot_a_res0af8_0619_first_je_audit.py`.
No native reach, C parity, or full-C count is claimed. Original media
and RAM stay ignored under `resources/`.
