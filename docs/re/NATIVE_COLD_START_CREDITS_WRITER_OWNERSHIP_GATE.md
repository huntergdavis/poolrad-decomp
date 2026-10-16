# Cold-start credits first box: static writer and source ownership

This source-only continuation narrows the first *possible* credits display
callee in the [credits source contract](COLD_START_CREDITS_SOURCE_CONTRACT.md)
to a specific guest memory-writing instruction and its nested return frames.
It does not observe the call in a running guest or identify the first live
writer of the credits screen. The credits asset, border, 33 printer calls,
and exact source-to-frame comparison remain owned by that separate contract;
the [codeword prompt](COLD_START_CODEWORD_SOURCE.md) follows the credits
return and is outside this call chain.

Before this audit, `timeout 20s deja "Poolrad U17 credits +0206 first visual
writer +022D RES 03F1 02E8 nested 0137 0000"` timed out without a result.
The [title-to-credits control gate](NATIVE_COLD_START_TITLE_CREDITS_CONTROL_GATE.md)
supplies the source caller to `+0206`; the
[rectangle-fill model](RES_0709_RECT_FILL_00B7.md) supplies the established
EGA page and FillChar ABI. All new spans below were checked again against
ignored DOS v1.3 originals and retained U17-entry RAM. No emulator was
started during the heavy-native hold.

## Pinned inputs and first-call bounds

| Ignored private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| native U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

U17's credits routine starts at `3909:0206`. Its 44-byte prologue through
the first box-call return, `+0206..+0232`, is original `GAME.OVR`
`[0x002DA,0x00306)`, SHA-256
`7b42fe759891f26e921bd881078234f2dc862ac6031715174afa8dff8ab5a6b4`;
loaded RAM hashes
`abb02784107333975835c362d803cb477a7fbad4f8e88609d4d7849343615724`.
Its five-byte far call at `+022D..+0232` is source file
`[0x00301,0x00306)`, original SHA-256
`f1c7615b48a308bf93bd3b12bd137453841816d9cf8d070481e87cb40399445b`,
loaded SHA-256
`e1d48e6e44315e0d78c8744ae728bd79dc9d58b614fdf111acbf805ada0fd834`.
The two changed bytes in that call are its relocated segment word.

Before this call, U17 pushes six low-byte rectangle arguments
`(1,1,38,3,0,0)`, copies the inline Pascal string at `CS:+004A` to a
stack-local destination through SYSTEM `+0B0B`, and passes that destination
to `RES_03F1:+02E8`. The existing credits contract establishes the empty
caption and inclusive cell rectangle `(1,1)..(38,3)`. This audit does not
attribute display writes to the preceding SYSTEM copy.

## Ordered EGA clear chain

| Caller instruction | Callee and static successor | Original / loaded SHA-256 |
| --- | --- | --- |
| U17 `+022D` | `RES_03F1:+02E8`; return IP `+0232` | `f1c7615b48a308bf93bd3b12bd137453841816d9cf8d070481e87cb40399445b` / `e1d48e6e44315e0d78c8744ae728bd79dc9d58b614fdf111acbf805ada0fd834` |
| `RES_03F1:+0311` | `PUSH CS; CALL +0137`; far return IP `+0315` | identical `3c18135cc6d768bf60ff388e1963eb64e11b4a47fe8275279a6f9eb5390b3c3a` |
| `RES_03F1:+01F1` | First EGA-page call to `RES_0709:+00B7`; return IP `+01F6` | `2286767d9eccf2e3fe83669ef5367f5ef97da8c2145e12d54b0291510e19093c` / `8d1b1dec8fbef9dea92ee3e851c55572850da6b639cd9fb3890c998e918cc8e2` |
| `RES_0709:+0140` | Far call to SYSTEM `RES_0AF8:+16A8`; return IP `+0145` | `adc5985dcb761563534658cb03a15d9afdf2305d7e2b82f800875d921f91ee3a` / `2776d6ebc232e7277e121d58341055aaa57b341e24b5edb8a4d116e421da1db3` |

`RES_03F1:+02E8..+0367` is 127 bytes including its `RETF 0x10` at
`+0364`, not merely the first byte of that return. Expanded START image
`[0x041F8,0x04277)` maps contiguously to file `[0x0423A,0x042B9)`;
original SHA-256 is
`71dfae69a38bb9aadad319291d7218cd65523ba14162fb45e5e699ca2d6816d1`,
loaded SHA-256 is
`c9e254e0f313763bbdafaba8ee76d60535e6835322468f5c85257b121badac1c`.
It calls `+0137` before `+0000` and before checking the copied string's
length. The first nested routine, `RES_03F1:+0137..+02E8`, is 433 bytes,
original SHA-256
`1258f9ea251527a82be56c60ecbd1f07e300c994bc0fdbac2a6537369080f5fc`.
Its `DS:4A1E=1` arm passes page byte 0 and color byte 0 to the first
`RES_0709:+00B7` call at `+01F1`; the subsequent `+020C` call passes page
byte 1 and the same color. The full rectangle helper
`+00B7..+0171` is 186 bytes, original SHA-256
`f26747c1e8836399f118763dfee5a2bdeace551495f75b5533772b888fa65ff5`,
loaded SHA-256
`02ba954ed5bc0baa194fa7e74eeaceba27b09386107ac51df7df27a09c6aaa5f`.

On that mode-1, first-page source arm, `RES_0709:+00B7` first selects the
page segment in `DS:4A18` and sets EGA controller registers through its
local callees. At `+0124..+0140` it passes `DS:4A18` as a far destination,
the inclusive byte width, and fill color to SYSTEM `+16A8`. The full
20-byte SYSTEM body is expanded image `[0x0C628,0x0C63C)`, START file
`[0x0C31E,0x0C332)`, source and loaded SHA-256
`f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7`.
It loads `ES:DI`, `CX`, and `AL` from its caller's stack, clears direction,
then executes `REP STOSB` at `RES_0AF8:+16B7` (loaded `131C:16B7`).
That two-byte instruction maps to START file `[0x0C32D,0x0C32F)` and is
identical in source and RAM, SHA-256
`50b2d3be54fb620b49268d3b4bd0c759531794c4c735fc2916071807cf9ee7a6`.
At a stop on it, the *static expected* innermost far return is
`0F2D:0145`; the caller frames above discriminate the first credits box.

The first call's computed page segment is `A000` if `DS:4A1E=1` and the
documented page selector receives page 0. Candidate segment `14A0` has
`DS:4A1E=1` and active page 0 at the retained U17-entry and return RAM
stops, but CPU `DS` and the intermediate page/plane state were not captured
at this call. The initial box clear is a **static candidate** to write the
credits background; later border and printer calls overwrite their own
regions. The direct `REP STOSB` site is proven from source, while its
execution, first live pixel write, and visible page are unmeasured.
No C translation, source-count change, or native parity claim is made here.
