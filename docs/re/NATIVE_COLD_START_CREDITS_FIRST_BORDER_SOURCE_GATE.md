# Cold-start credits first border: static source and writer boundary

This source-only gate follows the first credits box *after* its two EGA
rectangle clears. It identifies the first corner call, the direct-copy
overlay branch, and the resident instruction that could write its pixels.
It does not establish a live instruction stop, first changed pixel, or
visible page. The [credits source-to-frame contract](COLD_START_CREDITS_SOURCE_CONTRACT.md)
owns the art and final frame; the [first clear-writer gate](NATIVE_COLD_START_CREDITS_WRITER_OWNERSHIP_GATE.md)
owns the earlier `RES_0AF8:+16B7` candidate.

Before this audit, `timeout 20s deja "Poolrad cold-start credits first border
writer RES_03F1 0000 overlay 0428 002A RES_0464 1951 group4 static source
ownership"` timed out without a result. The
[shared box source map](COLD_START_CODEWORD_BOX_SOURCE.md),
[unit-0x40C entry-2 model](SLOT_A_U40C_FIRST_TILE_PLANE.md), and
[resident display source map](NATIVE_COLD_START_TITLE_CALLER_PAGE_GATE.md)
were reused as leads. All spans below were checked against the ignored
DOS v1.3 originals and retained U17-entry RAM; no emulator was started.

## First border call and source object

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

The first credits box calls `RES_03F1:+02E8` from U17 `+022D` with
inclusive cell rectangle `(1,1)..(38,3)` and style zero. Its wrapper
calls the rectangle clear at `+0312`, then `PUSH CS; CALL +0000` at
`+0329..+032D`, returning to `+032D`. That four-byte border transfer is
START file `[0x0427B,0x0427F)` and loaded `0C15:+0329..+032D`,
identical SHA-256
`c339676ccba3b4aa3cf25be0a29ece2f4cd74158c355984f4e34c730cc42f3a1`.
The full border body `RES_03F1:+0000..+0137` is 311 bytes, original
START `[0x03F52,0x04089)` SHA-256
`fae6cb41423bcd48237e75a08f5eda70642254eac8449373291d57f34120a8a4`;
loaded `0C15:+0000..+0137` SHA-256
`9182a5a6d13315f7ce869cc55a5903b5ec0bddd57327b6909aabdd1093dfe857`.

`+0006..+000F` expands the rectangle to `(0,0)..(39,4)`.
With style zero, `+0012..+0020` selects item `0x0114`, the first
group-4 corner. The first far call at `+0038..+003D` passes
`(x=0,y=0,item=0x0114,flag=0,flag=0)` to overlay stub `0428:002A`;
its return offset is `+003D`. Original START `[0x03F8A,0x03F8F)`
hashes `64940f6941110590c5125977fe8e435e68b57a1fc5b998a494f76b176f0050ec`;
loaded RAM hashes
`cca89ed9a112f415a0e76a1464e43fa7a8c7d6bb20c7265c54018bd62666141f`.
The later corner and edge loops are outside this first-call gate.

The established overlay mapping sends that stub to unit 0x40C entry 2,
`+015D..+02AC`, GAME.OVR `[0x32392,0x324E2)`, 336 bytes, SHA-256
`2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00`.
Item `0x0114` selects group 4 at `+01A6..+01B4`; the group-4 remap at
`DS:272A=0x0100` makes its index 20. Candidate `DS=14A0` in the retained
U17-entry RAM has `DS:272A=0x0100`, group-4 source pointer
`DS:5E4A=41E1:0008`, and destination pointer `DS:5E4E=3DE1:0000`.
These are entry-snapshot values, not CPU DS or object pointers captured
at the border call.

## Direct-copy and display boundary

The zero flag at entry-2 `[BP+8]` takes `+0216 -> +0250`, bypassing
the separate `+0249` offscreen tile branch used by saved gameplay.
The `+0250..+02A7` direct branch is GAME.OVR `[0x32485,0x324DC)`,
87 bytes, SHA-256
`09cef19207ace80b443cba52e7a2add5ea00dc777f19f525e509b32cfe80186b`.
It computes the selected source-object item address, copies one item to
the group destination through SYSTEM `RES_0AF8:+1685` at `+028B`, then
passes coordinates and that destination to `RES_0464:+1951` at `+02A2`.
The 35-byte SYSTEM copy body is START `[0x0C2FB,0x0C31E)`, SHA-256
`1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972`;
its `REP MOVSB` at `+16A1` writes the destination object, whose live
address was not stopped here. The five-byte entry-2 display call at
GAME.OVR `[0x324D7,0x324DC)` hashes
`6cc3bf5965381d41d3fb6117d07aba4d8e740e591ee4d4decca3cdf8e3690898`
and returns to entry-2 `+02A7` if the direct branch executes.

`RES_0464:+1951..+1B51` is 513 bytes in START
`[0x05FCA,0x061CB)`, original SHA-256
`d64bb18e4503a5679481894c5ddaad44b3c0f703b00d1810afbbba8e9f358de3`.
On its `DS:4A1E=1` EGA arm it selects plane masks and segment `A000`,
then stores a byte at `RES_0464:+1AA2` (loaded candidate `0C88:1AA2`).
That three-byte instruction at START `[0x0611B,0x0611E)` hashes
`7cc107bb3b15e14c0a2b41083f41a5748ab0fb6ac0bbd0122030d1da88bb4d92`.
The source-continuation offsets `+02A7` in entry 2, `+003D` in the
border helper, `+032D` in the wrapper, and `+0232` in U17 distinguish
this first-corner path from the remaining border calls. No stack at
the display store was captured.

The direct display store is a **static candidate** for the first border
pixel write after the two clears. Whether the overlay is loaded at this
call, which EGA branch executes, the first changed video byte, and its
visible-page effect all require a later native stop after the host hold.
This gate adds no C translation, full-game count, or native parity claim.
