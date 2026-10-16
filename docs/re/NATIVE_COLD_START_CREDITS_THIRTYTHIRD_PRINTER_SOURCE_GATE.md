# Cold-start credits thirty-third printer: final bounded caller and source gate

This source-only audit covers the final U17 printer caller `+05F8..+0615`, after the separately audited [thirty-second printer](NATIVE_COLD_START_CREDITS_THIRTYSECOND_PRINTER_SOURCE_GATE.md). The existing [terminal wait gate](NATIVE_COLD_START_CREDITS_TERMINAL_WAIT_SOURCE_GATE.md) starts at its static return `+0615`; there is no thirty-fourth printer caller. The [credits source contract](COLD_START_CREDITS_SOURCE_CONTRACT.md), [printer prefix](RESIDENT_0709_0626_FIRST_COPY_GATE.md), and [post-copy control](RESIDENT_0709_0626_POSTCOPY_CONTROL.md) supplied leads; the spans below were independently checked against ignored DOS v1.3 originals and retained U17-entry RAM. A bounded `deja` lookup timed out without an indexed result. No emulator was started under the host's heavy-native hold.

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

U17 is loaded at segment `3909`. The 29-byte `+05F8..+0615` caller maps to GAME.OVR `[0x006CC,0x006E9)`, original SHA-256 `4813ae6a07882012301a26465f6c0cfd49ad639ab3920da6d09b76ac6a72296d`, loaded SHA-256 `609942d25c007cc30e038911c36925f4ef18a6619920bb5630b209fb57d5ba15`. Original and loaded 16-bit instruction decoding agree on the operand setup; only caller-relative offsets `0x16,0x17,0x1B,0x1C` differ, in the two far-call segment words:

| U17 offset | Decoded caller operation |
| --- | --- |
| `+05F8..+0601` | Set AL to 2, 23, and 11, pushing AX after each: low-byte column, row, and color. |
| `+0601..+0606` | Push local copy-destination pointer `SS:BP-0x24`. |
| `+0606..+060B` | Push inline Pascal source pointer `CS:+01E2`. |
| `+060B..+0610` | Far-call SYSTEM `0AF8:0B0B`, loaded `131C:0B0B`; static return `+0610`. |
| `+0610..+0615` | Far-call resident printer `0709:0626`, loaded `0F2D:0626`; static return `+0615`. |

The SYSTEM call occupies original GAME.OVR `[0x006DF,0x006E4)` SHA-256 `7780859e922a704a7253291b792982fced470c34e06c47f1daf92468f6774bae`, loaded SHA-256 `1b227ae15a60114aee8a68adef67dd8cad844a7d284497c578ca3160566b5a9b`. The printer call occupies original `[0x006E4,0x006E9)` SHA-256 `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23`, loaded SHA-256 `cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`. The source `CS:+01E2` is original `[0x002B6,0x002DA)` and identical in RAM: 36 bytes, SHA-256 `233db9ae950c15a2484b8cbfa1130f958c762403f48c52f3b54bcdbb02db090c`. Its Pascal length is 35; no game text is reproduced. The source ends at the numeric start of the separate U17 `+0206` procedure. The record size matches the span from `BP-0x24` to BP, but this alone does not establish runtime stack safety. Thirty-five cells from column 2 occupy columns 2–36, within the 40-column row.

The shared printer `RES_0709:+0626..+06A7` is 129 bytes, original START `[0x076EF,0x07770)` SHA-256 `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`, loaded SHA-256 `8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`. Its nested SYSTEM copy at `+063C` returns at `+0641`, its first glyph `PUSH CS; CALL +01DF` at `+0690..+0694` returns at `+0694`, and its `RETF 0x0A` at `+06A4` would return to U17 `+0615` if the nested calls complete. The terminal wait transfer begins at `+0615`, but this source gate does not establish that a native run reached either call.

The first host-visible title-to-credits demo in #511 still needs a reached native writer/page boundary and a C route; this source note does not establish either. The final call's runtime stack contents, glyph branch, EGA page, and framebuffer effects remain unobserved. This gate adds no live writer, C translation, full-frame parity, or game-function-count claim.
