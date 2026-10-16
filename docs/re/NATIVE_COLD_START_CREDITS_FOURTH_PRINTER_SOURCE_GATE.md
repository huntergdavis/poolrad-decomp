# Cold-start credits fourth printer: bounded source gate

This source-only audit covers U17 `+02AF..+02CC`, the credits printer caller immediately after the separately audited third call. The [credits source contract](COLD_START_CREDITS_SOURCE_CONTRACT.md), [printer prefix](RESIDENT_0709_0626_FIRST_COPY_GATE.md), and [post-copy control](RESIDENT_0709_0626_POSTCOPY_CONTROL.md) notes supplied leads. Caller, source, and callsite spans were checked against ignored DOS v1.3 originals and retained U17-entry RAM. A bounded `deja` query for this fourth caller/source/return path found no indexed session match. No emulator was started under the host's heavy-native hold.

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

U17 is loaded at segment `3909`. The 29-byte `+02AF..+02CC` caller maps to GAME.OVR `[0x00383,0x003A0)`, original SHA-256 `a2f1ecd849b123c64badaf167b57dc15d8cc4aba3543728e97e6cf269db17d2b`, loaded SHA-256 `b4309e986ad83d7d2fdabac93ca9e19107e2e66c3ccab8734a9f1003e8a7b227`. Its decoded operations are:

| U17 offset | Operation and boundary |
| --- | --- |
| `+02AF..+02B8` | Push AX three times after setting AL to 2, 3, and 10: column, row, and color. |
| `+02B8..+02BD` | Push local destination pointer `SS:BP-0x25`. |
| `+02BD..+02C2` | Push inline Pascal source pointer `CS:+0073`. |
| `+02C2..+02C7` | Far-call SYSTEM `0AF8:0B0B` to copy the source; static return `+02C7`. |
| `+02C7..+02CC` | Far-call resident printer `0709:0626`, loaded target `0F2D:0626`; static return `+02CC`. |

The SYSTEM call is original GAME.OVR `[0x00396,0x0039B)` SHA-256 `7780859e922a704a7253291b792982fced470c34e06c47f1daf92468f6774bae`, loaded SHA-256 `1b227ae15a60114aee8a68adef67dd8cad844a7d284497c578ca3160566b5a9b`. The printer call is original `[0x0039B,0x003A0)` SHA-256 `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23`, loaded SHA-256 `cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`. The source at `CS:+0073` is original `[0x00147,0x0016C)` and identical in RAM: 37 bytes, SHA-256 `3ee6292b9254fc15e1dc09756c1caf0a91fd5c3c4379ce827a7fd77316bd680d`. Its Pascal length is 36; no game text is reproduced. Column 2 plus 36 cells ends at column 37, within the 40-column row. The source record starts where the third call's distinct record ends.

The shared printer `RES_0709:+0626..+06A7` is 129 bytes, original START `[0x076EF,0x07770)` SHA-256 `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`, loaded SHA-256 `8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`. Its `+063C` SYSTEM `+0B25` copy returns at `+0641`; its first glyph call `PUSH CS; CALL +01DF` at `+0690..+0694` returns at `+0694`, and its `RETF 0x0A` at `+06A4` returns to U17 `+02CC` if the calls complete. This is a static nested boundary, not a fourth-call native trace.

The fourth call's runtime DS source, glyph branch, EGA page, and framebuffer effects remain unobserved. This audit adds no live-writer, C translation, full-frame parity, or game-function-count claim.
