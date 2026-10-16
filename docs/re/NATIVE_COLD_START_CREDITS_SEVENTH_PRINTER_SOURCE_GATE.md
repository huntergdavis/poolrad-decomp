# Cold-start credits seventh printer: bounded caller and source gate

This source-only audit covers U17 `+0306..+0323`, the caller immediately after the merged [sixth printer gate](NATIVE_COLD_START_CREDITS_SIXTH_PRINTER_SOURCE_GATE.md). The eighth caller starts at `+0323` and has a distinct record. The [credits source contract](COLD_START_CREDITS_SOURCE_CONTRACT.md), [printer prefix](RESIDENT_0709_0626_FIRST_COPY_GATE.md), and [post-copy control](RESIDENT_0709_0626_POSTCOPY_CONTROL.md) supplied leads; every span below was checked against ignored DOS v1.3 originals and retained U17-entry RAM. Bounded `deja` searches for this caller/source path timed out without a result. No emulator was started under the host's heavy-native hold.

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

U17 is loaded at segment `3909`. The 29-byte `+0306..+0323` caller maps to GAME.OVR `[0x003DA,0x003F7)`, original SHA-256 `c33f168ac2d710324f983cd5a2753c477c19bfe7b3b0b4878ade12f8245709eb`, loaded SHA-256 `7a0d766cd8b687203d1b1f36893c15f713a3be5ff1c2c5262cccff9685ec6139`. Original and loaded instruction decoding agree on the operand setup; only the far-call segments relocate:

| U17 offset | Decoded caller operation |
| --- | --- |
| `+0306..+030F` | Set AL to 14, 6, and 11, pushing AX after each: low-byte column, row, and color. |
| `+030F..+0314` | Push local copy-destination pointer `SS:BP-0x0C`. |
| `+0314..+0319` | Push inline Pascal source pointer `CS:+00BE`. |
| `+0319..+031E` | Far-call SYSTEM `0AF8:0B0B`, loaded `131C:0B0B`; static return `+031E`. |
| `+031E..+0323` | Far-call resident printer `0709:0626`, loaded `0F2D:0626`; static return `+0323`. |

The SYSTEM call occupies original GAME.OVR `[0x003ED,0x003F2)` SHA-256 `7780859e922a704a7253291b792982fced470c34e06c47f1daf92468f6774bae`, loaded SHA-256 `1b227ae15a60114aee8a68adef67dd8cad844a7d284497c578ca3160566b5a9b`. The printer call occupies original `[0x003F2,0x003F7)` SHA-256 `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23`, loaded SHA-256 `cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`. The source `CS:+00BE` is original `[0x00192,0x0019E)` and identical in RAM: 12 bytes, SHA-256 `01db17b2e5e4fe5e74db5a70bafc8cb588de725b6348e28b2f485dd4c7a42734`. Its Pascal length is 11; no game text is reproduced. The record size matches the numeric span from `BP-0x0C` to BP, but this alone does not establish runtime stack safety. Column 14 plus 11 cells ends at column 24, within the 40-column row. The eighth record begins at `CS:+00CA`, where this one ends.

The shared printer `RES_0709:+0626..+06A7` is 129 bytes, original START `[0x076EF,0x07770)` SHA-256 `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`, loaded SHA-256 `8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`. Its nested SYSTEM copy at `+063C` returns at `+0641`, its first glyph `PUSH CS; CALL +01DF` at `+0690..+0694` returns at `+0694`, and its `RETF 0x0A` at `+06A4` would return to U17 `+0323` if the nested calls complete. These are static call boundaries, not a seventh-call native trace.

The seventh call's runtime stack contents, glyph branch, EGA page, and framebuffer effects remain unobserved. This gate adds no live writer, C translation, full-frame parity, or game-function-count claim.
