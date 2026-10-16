# Cold-start credits third printer: bounded source gate

This source-only audit follows the [second printer gate](NATIVE_COLD_START_CREDITS_SECOND_PRINTER_SOURCE_GATE.md) and covers the next distinct U17 caller at `+0292..+02AF`. The [credits source contract](COLD_START_CREDITS_SOURCE_CONTRACT.md) and [printer prefix](RESIDENT_0709_0626_FIRST_COPY_GATE.md) and [post-copy control](RESIDENT_0709_0626_POSTCOPY_CONTROL.md) notes supplied leads; the caller, source, and callsite spans below were checked again against ignored DOS v1.3 originals and retained U17-entry RAM. A bounded `deja` query for this caller/source/return path found no indexed session match. No emulator was started under the host's heavy-native hold.

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

U17 is loaded at segment `3909`. The 29-byte `+0292..+02AF` caller maps to GAME.OVR `[0x00366,0x00383)`, original SHA-256 `d26bda2f11d3d2b7d0344296e7b2e22b3a36166d790b5a2b7f0aa25e698a7058`, loaded SHA-256 `a913459a9fabcc4ac737a39992cc93afdf4393cf454d15f13eef5781156bfee5`. Its decoded operations are:

| U17 offset | Operation and boundary |
| --- | --- |
| `+0292..+029B` | Push AX three times after setting AL to 15, 2, and 10: column, row, and color. |
| `+029B..+02A0` | Push local destination pointer `SS:BP-0x09`. |
| `+02A0..+02A5` | Push inline Pascal source pointer `CS:+006A`. |
| `+02A5..+02AA` | Far-call SYSTEM `0AF8:0B0B` to copy the source; static return `+02AA`. |
| `+02AA..+02AF` | Far-call resident printer `0709:0626`, loaded target `0F2D:0626`; static return `+02AF`. |

The SYSTEM call is original GAME.OVR `[0x00379,0x0037E)` SHA-256 `7780859e922a704a7253291b792982fced470c34e06c47f1daf92468f6774bae`, loaded SHA-256 `1b227ae15a60114aee8a68adef67dd8cad844a7d284497c578ca3160566b5a9b`. The printer call is original `[0x0037E,0x00383)` SHA-256 `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23`, loaded SHA-256 `cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`. The source at `CS:+006A` is original `[0x0013E,0x00147)` and identical in RAM: nine bytes, SHA-256 `2b133a166f667a4203d563575a07db7cc9c75704f3d1deebfff627d4a674b37e`. Its Pascal length is eight; no game text is reproduced. Column 15 plus eight cells lies within the 40-column row, conditional on normal nested returns.

The shared printer `RES_0709:+0626..+06A7` is 129 bytes, original START `[0x076EF,0x07770)` SHA-256 `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`, loaded SHA-256 `8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`. Its `+063C` SYSTEM `+0B25` copy returns at `+0641`; its first glyph call `PUSH CS; CALL +01DF` at `+0690..+0694` returns at `+0694`, and its `RETF 0x0A` at `+06A4` returns to U17 `+02AF` if the calls complete. This repeats a verified static printer boundary, not a third-call native trace.

The third call's runtime DS source, glyph branch, EGA page, and framebuffer effects remain unobserved. This audit adds no live-writer, C translation, full-frame parity, or game-function-count claim.
