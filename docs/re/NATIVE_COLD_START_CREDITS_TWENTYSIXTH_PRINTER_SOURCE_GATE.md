# Cold-start credits twenty-sixth printer: bounded caller and source gate

This source-only audit covers U17 `+052D..+054A`, after the separately audited twenty-fifth caller at `+0510..+052D`. The next caller starts at `+054A` and is outside this gate. The [credits source contract](COLD_START_CREDITS_SOURCE_CONTRACT.md), [printer prefix](RESIDENT_0709_0626_FIRST_COPY_GATE.md), and [post-copy control](RESIDENT_0709_0626_POSTCOPY_CONTROL.md) supplied leads; the spans below were independently checked against ignored DOS v1.3 originals and retained U17-entry RAM. A bounded `deja` query for this pair returned no indexed match. No emulator was started under the host's heavy-native hold.

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

U17 is loaded at segment `3909`. The 29-byte `+052D..+054A` caller maps to GAME.OVR `[0x00601,0x0061E)`, original SHA-256 `c45871e71502a788edef2707de0947b9708cee852702275055756c4ee1ee8054`, loaded SHA-256 `e53f9edba6c322b9286406e1befdc7d9957796e2f9d395199fbef0b4501023a1`. Original and loaded 16-bit instruction decoding agree on the operand setup; only caller-relative offsets `0x16,0x17,0x1B,0x1C` differ, in the two far-call segment words:

| U17 offset | Decoded caller operation |
| --- | --- |
| `+052D..+0536` | Set AL to 20, 17, and 11, pushing AX after each: low-byte column, row, and color. |
| `+0536..+053B` | Push local copy-destination pointer `SS:BP-0x0D`. |
| `+053B..+0540` | Push inline Pascal source pointer `CS:+0184`. |
| `+0540..+0545` | Far-call SYSTEM `0AF8:0B0B`, loaded `131C:0B0B`; static return `+0545`. |
| `+0545..+054A` | Far-call resident printer `0709:0626`, loaded `0F2D:0626`; static return `+054A`. |

The SYSTEM call occupies original GAME.OVR `[0x00614,0x00619)` SHA-256 `7780859e922a704a7253291b792982fced470c34e06c47f1daf92468f6774bae`, loaded SHA-256 `1b227ae15a60114aee8a68adef67dd8cad844a7d284497c578ca3160566b5a9b`. The printer call occupies original `[0x00619,0x0061E)` SHA-256 `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23`, loaded SHA-256 `cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`. The source `CS:+0184` is original `[0x00258,0x00265)` and identical in RAM: 13 bytes, SHA-256 `f4d8f2ec1cb4de5f9a7d5be3d3817359f6c8b6138057f89b6a19bb6b4f597dd3`. Its Pascal length is 12; no game text is reproduced. The record size matches the numeric span from `BP-0x0D` to BP, but this alone does not establish runtime stack safety. Twelve cells from column 20 occupy columns 20–31, within the 40-column row.

The shared printer `RES_0709:+0626..+06A7` is 129 bytes, original START `[0x076EF,0x07770)` SHA-256 `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`, loaded SHA-256 `8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`. Its nested SYSTEM copy at `+063C` returns at `+0641`, its first glyph `PUSH CS; CALL +01DF` at `+0690..+0694` returns at `+0694`, and its `RETF 0x0A` at `+06A4` would return to U17 `+054A` if the nested calls complete. These are static call boundaries, not a twenty-sixth-call native trace.

The first host-visible title-to-credits demo in #511 still needs a reached native writer/page boundary and a C route; this source note does not establish either. The twenty-sixth call's runtime stack contents, glyph branch, EGA page, and framebuffer effects remain unobserved. This gate adds no live writer, C translation, full-frame parity, or game-function-count claim.
