# Cold-start title-to-credits source control gate

This source-only gate maps the conditional path from unit `0x17`'s title
display call to its separate `+0206` credits-composition routine. The
[continuous native sample](NATIVE_COLD_START_SCREEN_GAPS.md) orders the stable
title RGB SHA-256
`3714eba3761b6d8520c61b61bc60b844dc63a375fa9c2bad166dee509a15e2d8`
before the credits RGB SHA-256
`624f69b5f7bce90ba859eee24aee2e4ac97daf66c405b145f20534123eb28698`.
The separate [native U17 call/return](NATIVE_COLD_START_U17_PICTURE_ROUTE.md)
brackets credits visible at resident `0824:0122`. Neither run stopped inside
the title-to-credits transition, so the decoded path below is not a same-run
instruction trace or first-writer measurement.

Before this audit, `timeout 20s deja "Poolrad cold start title credits
transition U17 title to credits first framebuffer writer caller control"`
timed out without a result. The
[U17 title source gate](NATIVE_COLD_START_TITLE_SOURCE_GATE.md),
[branch-flag producer](NATIVE_COLD_START_TITLE_FLAG_PRODUCER_GATE.md),
[resident loader map](RESIDENT_LOADER_MAP.md), and the
[video-mode gate](RESIDENT_VIDEO_INIT_4426.md) supplied prior bounds.
No emulator was started under the heavy-native hold.

## Pinned source and loaded control span

| Ignored private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| native U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

Unit `0x17` original code begins at `GAME.OVR` file `0x000D4`; entry 1
starts at relative `+0627`. The bounded `+0690..+0809` control span is 377
bytes, source file `[0x00764,0x008DD)`, SHA-256
`853aa30cde8cabd6f8de4c320c37bf76f06cce4f10685a0012249f8092ecc6f0`.
At loaded `3909:0690..0809` it hashes
`a0f334e22dac8c5d5041ad5357e6ec3cae3a8769654b7c8e760ce2c4eac298b8`.
The 46 changed bytes are confined to 23 descriptor-listed far-call
relocation words; there are zero differences outside those words.

## Conditional path to credits composition

| U17 relative site | Decoded source action and successor |
| --- | --- |
| `+0690..+0697` | Compare `DS:49D1` with zero; nonzero jumps to `+06AD`. The zero arm calls runtime `0AF8:+0B25` at `+06A5`, then jumps from `+06AA` to the outer epilogue `+0805`, bypassing the path below. The seven-byte compare/JNE hashes `349191955f710cbf09de2d115647653dc4ac17004d72946ee1268689d6d02031` in source and RAM. |
| `+06AD..+06F3` | Construct the block-2 request for the same inline archive name as the first image, call `RES_0464:+0111` at `+06CA`, then call `RES_0464:+1951` at `+06EE` to display the bitmap. These are static call sites, not observed display stops. |
| `+06F3..+070E` | Release the bitmap slot through `RES_0464:+2595` at `+06F8`; call `RES_0709:+0C3F` at `+06FD`; add `0x01F4` to its returned `DX:AX` and save the resulting two words in the local frame. The callee's bounded source returns `DX:AX` at `+0CA4/+0CA7`; its full timing semantics are outside this gate. |
| `+070E..+07DC` | If `DS:4A1E` differs from 1, jump to `+07DC`. Otherwise a guarded loop uses runtime `0AF8:+11C0`, resident palette setter `RES_0464:+2A0D`, and delay `RES_0A40:+029C`; several conditional jumps also lead to `+07DC`. Palette/device effects and loop trip counts were not observed. |
| `+07DC..+07FC` | Call `RES_0A40:+02FD`; nonzero `AL` exits to `+07FC`. If zero, call `RES_0709:+0C3F` again and compare its returned `DX:AX` with the saved pair; a lesser value returns to `+070E`, otherwise fall through to `+07FC`. The earlier loader audit identifies `+02FD` as a keyboard-state poll; no key result was captured here. |
| `+07FC..+0805` | Call `RES_0899:+0104`, then `PUSH CS; CALL +0206`. On direct entry `RES_0899:+0104` jumps from `+010A` to its `+011E` epilogue and far-returns without a direct display store. |
| `+0805..+0809` | Restore the outer frame and `RETF` to the resident caller. The independent native run observed the U17 return at `0824:0122`, with credits visible, but did not retain this local call's PC or stack frame. |

The four-byte `PUSH CS; CALL +0206` at U17 `+0801..+0805` is identical in
source file `[0x008D5,0x008D9)` and loaded RAM, SHA-256
`67ded38815f34cc1edbbef3bf1d7eea2255d8ab76e16e5337b8cfd9c85e7fc29`.
The near call pushes return IP `+0805` above the pushed CS, forming the
far-return frame consumed by `+0206`'s `RETF` at `+0620`. The callee starts
with a prologue at `+0206`; its first 26 bytes, original file
`[0x002DA,0x002F4)`, equal loaded RAM and hash
`d6a8560c9a80ba0db4b951f23a1ef7c42c1dd56d47c5e984c8849261e7009510`.
The four-byte `+061D..+0621` epilogue equals source and RAM, SHA-256
`d938592002da1e8f84eae35df938d5b693d827276f6e045d142340640128831a`.
Inline Pascal data at `+0621..+0627` follows the return and is outside the
executable callee body.

This map does not prove that native execution took `DS:49D1`'s nonzero arm,
that it reached `+0206`, or which instruction first changed the credits
screen. A separate first-call ownership gate addresses the static visual
callee without assigning native writer priority.
No C parity or game-function count changes.
