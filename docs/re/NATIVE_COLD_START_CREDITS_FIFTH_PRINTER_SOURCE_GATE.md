# Cold-start credits fifth printer: bounded caller and source gate

This source-only audit ranks U17 `+02CC..+02E9` first after the merged [fourth printer gate](NATIVE_COLD_START_CREDITS_FOURTH_PRINTER_SOURCE_GATE.md): it is the immediately following caller, with its own source record and static return. The sixth caller begins at `+02E9` and is a separate gate. The [credits source contract](COLD_START_CREDITS_SOURCE_CONTRACT.md) and [printer source gates](RESIDENT_0709_0626_FIRST_COPY_GATE.md) supplied leads; the spans below were checked against ignored DOS v1.3 originals and retained U17-entry RAM. A bounded `deja` search for this fifth caller/source/frame found no indexed session match. No emulator was started under the host's heavy-native hold.

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

U17 is loaded at segment `3909`. The 29-byte `+02CC..+02E9` caller maps to GAME.OVR `[0x003A0,0x003BD)`, original SHA-256 `2b39fb49a37f859fb36f9c00249c650e61e1efc9f75d0348d44a0e39d9058a9e`, loaded SHA-256 `2641c0017d13b8d6f0ccaed28cd52d251136ef38d3e0c26cb37d54dce7ae5fc2`. Original and loaded instruction decoding agree on the operand setup; relocation changes the far-call segments:

| U17 offset | Decoded caller operation |
| --- | --- |
| `+02CC..+02D5` | Set AL to 1, 5, and 10, pushing AX after each: low-byte column, row, and color. |
| `+02D5..+02DA` | Push local copy-destination pointer `SS:BP-0x11`. |
| `+02DA..+02DF` | Push inline Pascal source pointer `CS:+0098`. |
| `+02DF..+02E4` | Far-call SYSTEM `0AF8:0B0B`, loaded `131C:0B0B`; static return `+02E4`. |
| `+02E4..+02E9` | Far-call resident printer `0709:0626`, loaded `0F2D:0626`; static return `+02E9`. |

The SYSTEM call occupies original GAME.OVR `[0x003B3,0x003B8)` SHA-256 `7780859e922a704a7253291b792982fced470c34e06c47f1daf92468f6774bae`, loaded SHA-256 `1b227ae15a60114aee8a68adef67dd8cad844a7d284497c578ca3160566b5a9b`. The printer call occupies original `[0x003B8,0x003BD)` SHA-256 `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23`, loaded SHA-256 `cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`. The source `CS:+0098` is original `[0x0016C,0x0017D)` and identical in RAM: 17 bytes, SHA-256 `e2ca563eb61ec76ee8b685773449f18e27758d5c09beb7a703a230598ae13475`. Its Pascal length is 16; no game text is reproduced. The record size matches the numeric span from `BP-0x11` to BP; this alone does not establish runtime stack safety. Column 1 plus 16 cells ends at column 16, inside the 40-column row. The next source record begins at `CS:+00A9`, where this one ends.

The shared printer `RES_0709:+0626..+06A7` is 129 bytes, original START `[0x076EF,0x07770)` SHA-256 `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`, loaded SHA-256 `8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`. Its nested SYSTEM copy at `+063C` returns at `+0641`, its first glyph `PUSH CS; CALL +01DF` at `+0690..+0694` returns at `+0694`, and its `RETF 0x0A` at `+06A4` would return to U17 `+02E9` if the nested calls complete. These are static call boundaries, not a fifth-call native trace.

The fifth call's runtime stack contents, glyph branch, EGA page, and framebuffer effects remain unobserved. This gate adds no live writer, C translation, full-frame parity, or game-function-count claim.
