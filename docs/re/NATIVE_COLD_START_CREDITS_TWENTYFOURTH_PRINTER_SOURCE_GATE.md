# Cold-start credits twenty-fourth printer: bounded caller and source gate

This source-only audit covers U17 `+04F3..+0510`, after the separately audited twenty-third caller at `+04D6..+04F3`. The next caller starts at `+0510` and is outside this gate. The [credits source contract](COLD_START_CREDITS_SOURCE_CONTRACT.md), [printer prefix](RESIDENT_0709_0626_FIRST_COPY_GATE.md), and [post-copy control](RESIDENT_0709_0626_POSTCOPY_CONTROL.md) supplied leads; the spans below were independently checked against ignored DOS v1.3 originals and retained U17-entry RAM. A bounded `deja` query did not return before its timeout; no indexed result was reused. No emulator was started under the host's heavy-native hold.

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

U17 is loaded at segment `3909`. The 29-byte `+04F3..+0510` caller maps to GAME.OVR `[0x005C7,0x005E4)`, original SHA-256 `fcc9dcaa297b7582025b93be5022aa987956166c87bed0bdcc5fa4007a057028`, loaded SHA-256 `e227ebb636811ee4804b225de99fcc41b229d5c37438df92ab99d5aa65743cd0`. Original and loaded 16-bit instruction decoding agree on the operand setup; only caller-relative offsets `0x16,0x17,0x1B,0x1C` differ, in the two far-call segment words:

| U17 offset | Decoded caller operation |
| --- | --- |
| `+04F3..+04FC` | Set AL to 20, 16, and 11, pushing AX after each: low-byte column, row, and color. |
| `+04FC..+0501` | Push local copy-destination pointer `SS:BP-0x0E`. |
| `+0501..+0506` | Push inline Pascal source pointer `CS:+0167`. |
| `+0506..+050B` | Far-call SYSTEM `0AF8:0B0B`, loaded `131C:0B0B`; static return `+050B`. |
| `+050B..+0510` | Far-call resident printer `0709:0626`, loaded `0F2D:0626`; static return `+0510`. |

The SYSTEM call occupies original GAME.OVR `[0x005DA,0x005DF)` SHA-256 `7780859e922a704a7253291b792982fced470c34e06c47f1daf92468f6774bae`, loaded SHA-256 `1b227ae15a60114aee8a68adef67dd8cad844a7d284497c578ca3160566b5a9b`. The printer call occupies original `[0x005DF,0x005E4)` SHA-256 `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23`, loaded SHA-256 `cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`. The source `CS:+0167` is original `[0x0023B,0x00249)` and identical in RAM: 14 bytes, SHA-256 `2c4d96a6fcfe7e7a3584eb24a4cc88aa93425c11f1e79d7b0c514b4443071547`. Its Pascal length is 13; no game text is reproduced. The record size matches the numeric span from `BP-0x0E` to BP, but this alone does not establish runtime stack safety. Thirteen cells from column 20 occupy columns 20–32, within the 40-column row.

The shared printer `RES_0709:+0626..+06A7` is 129 bytes, original START `[0x076EF,0x07770)` SHA-256 `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`, loaded SHA-256 `8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`. Its nested SYSTEM copy at `+063C` returns at `+0641`, its first glyph `PUSH CS; CALL +01DF` at `+0690..+0694` returns at `+0694`, and its `RETF 0x0A` at `+06A4` would return to U17 `+0510` if the nested calls complete. These are static call boundaries, not a twenty-fourth-call native trace.

The first host-visible title-to-credits demo in #511 still needs a reached native writer/page boundary and a C route; this source note does not establish either. The twenty-fourth call's runtime stack contents, glyph branch, EGA page, and framebuffer effects remain unobserved. This gate adds no live writer, C translation, full-frame parity, or game-function-count claim.
