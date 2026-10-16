# Cold-start credits sixth printer: bounded caller and source gate

This source-only audit covers U17 `+02E9..+0306`, the printer caller after the separately audited fifth call. Its record begins at `CS:+00A9`, where the fifth record ends; its caller, copy, and printer callsites have distinct original file offsets. The [credits source contract](COLD_START_CREDITS_SOURCE_CONTRACT.md) and [printer prefix](RESIDENT_0709_0626_FIRST_COPY_GATE.md) and [post-copy control](RESIDENT_0709_0626_POSTCOPY_CONTROL.md) notes supplied leads. All spans below were checked against ignored DOS v1.3 originals and retained U17-entry RAM. A bounded `deja` query for this sixth caller/source/frame found no indexed session match. No emulator was started under the host's heavy-native hold.

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

U17 is loaded at segment `3909`. The 29-byte `+02E9..+0306` caller maps to GAME.OVR `[0x003BD,0x003DA)`, original SHA-256 `b0f4da3f1a9b9bad9f19baadacd544fd92d3dac99c4c5242c7b12f4ac419bc88`, loaded SHA-256 `ae53c394b40a993a0523f3c1d20df2039a57907141c1923b7f1db0e9d17e9ff3`. Original and loaded decoding agree on the operands and instruction boundaries; the far-call segments relocate in RAM:

| U17 offset | Decoded caller operation |
| --- | --- |
| `+02E9..+02F2` | Set AL to 18, 5, and 14, pushing AX after each: low-byte column, row, and color. |
| `+02F2..+02F7` | Push local copy-destination pointer `SS:BP-0x15`. |
| `+02F7..+02FC` | Push inline Pascal source pointer `CS:+00A9`. |
| `+02FC..+0301` | Far-call SYSTEM `0AF8:0B0B`, loaded `131C:0B0B`; static return `+0301`. |
| `+0301..+0306` | Far-call resident printer `0709:0626`, loaded `0F2D:0626`; static return `+0306`. |

The SYSTEM call occupies original GAME.OVR `[0x003D0,0x003D5)` SHA-256 `7780859e922a704a7253291b792982fced470c34e06c47f1daf92468f6774bae`, loaded SHA-256 `1b227ae15a60114aee8a68adef67dd8cad844a7d284497c578ca3160566b5a9b`. The printer call occupies original `[0x003D5,0x003DA)` SHA-256 `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23`, loaded SHA-256 `cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`. The source `CS:+00A9` is original `[0x0017D,0x00192)` and identical in RAM: 21 bytes, SHA-256 `dc68b70182b423755b5d45dc7e1c94538ea24c9b3739b6fd648e436dba7a30ea`. Its Pascal length is 20; no game text is reproduced. The record size matches the numeric span from `BP-0x15` to BP, but that alone does not establish runtime stack safety. Column 18 plus 20 cells ends at column 37, within the 40-column row. The fifth caller covers columns 1 through 16 on the same row; this sixth placement starts at 18, with column 17 between them.

The shared printer `RES_0709:+0626..+06A7` is 129 bytes, original START `[0x076EF,0x07770)` SHA-256 `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`, loaded SHA-256 `8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`. Its nested SYSTEM copy at `+063C` returns at `+0641`, its first glyph `PUSH CS; CALL +01DF` at `+0690..+0694` returns at `+0694`, and its `RETF 0x0A` at `+06A4` would return to U17 `+0306` if the nested calls complete. These are static call boundaries, not a sixth-call native trace.

The sixth call's runtime stack contents, glyph branch, EGA page, and framebuffer effects remain unobserved. This gate adds no live writer, C translation, full-frame parity, or game-function-count claim.
