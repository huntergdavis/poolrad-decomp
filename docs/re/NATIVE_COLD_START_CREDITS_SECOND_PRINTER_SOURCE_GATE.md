# Cold-start credits second printer: bounded source gate

This source-only audit follows the [first printer gate](NATIVE_COLD_START_CREDITS_FIRST_PRINTER_SOURCE_GATE.md). It covers the next U17 caller setup and far call at `+0275..+0292`, its inline source, and the printer's static copy, glyph, and return boundaries. It does not establish a native stop at this call, a first display writer, or C/full-frame parity. The separate [credits source contract](COLD_START_CREDITS_SOURCE_CONTRACT.md) and [printer prefix](RESIDENT_0709_0626_FIRST_COPY_GATE.md) and [post-copy control](RESIDENT_0709_0626_POSTCOPY_CONTROL.md) notes supplied leads; every span below was rechecked against ignored DOS v1.3 originals and retained U17-entry RAM. A bounded `deja` search for the second printer/source/return path found no indexed session match. No emulator was started under the host's heavy-native hold.

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

U17 is loaded at segment `3909`. Its `+0275..+0292` setup and call is 29 bytes, original GAME.OVR `[0x00349,0x00366)` SHA-256 `4265d9a4c3ffbf59f6c49074fbba948c6e4de9b39610aba736c3af2a749b360f`; loaded SHA-256 `2c9bec3f59900b8a2ca2a5d240e246863f85f5bd5a63b1b21d705c1413dfcd79`. Decoding the complete span gives these caller operations:

| U17 offset | Operation and boundary |
| --- | --- |
| `+0275..+027E` | Push AX three times after setting AL to `0x1A`, `1`, and `0x0E`: column 26, row 1, color 14. |
| `+027E..+0283` | Compute `SS:BP-0x0A` and push its far pointer as the local copy destination. |
| `+0283..+0288` | Push `CS:+0060`, the inline Pascal source pointer. |
| `+0288..+028D` | Far-call SYSTEM `0AF8:0B0B`; static return `+028D`. |
| `+028D..+0292` | Far-call resident `0709:0626`; static return `+0292`. The loaded target is `0F2D:0626`. |

The SYSTEM call occupies original `[0x0035C,0x00361)` SHA-256 `7780859e922a704a7253291b792982fced470c34e06c47f1daf92468f6774bae` and loaded SHA-256 `1b227ae15a60114aee8a68adef67dd8cad844a7d284497c578ca3160566b5a9b`. The printer call occupies original `[0x00361,0x00366)` SHA-256 `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23` and loaded SHA-256 `cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`. The inline source at `CS:+0060` occupies original `[0x00134,0x0013E)` and is identical in RAM: 10 bytes, SHA-256 `6ebd8207c21836dbfb8237afa86fd258d2903c0398a5f5389c30a5fb25d326f5`. Its first byte specifies nine data bytes; none are reproduced here. Column 26 plus nine cells stays within 40 columns. Those are static source bounds, conditional on the prior copy and printer calls returning normally.

The far printer is `RES_0709:+0626..+06A7`, 129 bytes, original START `[0x076EF,0x07770)` SHA-256 `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`; loaded `0F2D:+0626..+06A7` SHA-256 `8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`. Its `+063C` SYSTEM `+0B25` copy returns at `+0641` if successful. Its column/row guards accept 26 and 1. In the glyph loop, `PUSH CS; CALL +01DF` occupies `+0690..+0694`, original and loaded SHA-256 `c6612c7266fcdf0defe91d5cf5d45a77c0d2e3d1056fcf31a0da6a76f3d9d4cf`; the static glyph return is `+0694`. The printer's `RETF 0x0A` at `+06A4` returns to U17 `+0292` after its loop, assuming its nested calls complete.

This gate records call operands, source length, and instruction boundaries only. The glyph's runtime DS source, EGA page, branch outcome, and framebuffer effects at the second call are unobserved. It adds no C translation or game-function count.
