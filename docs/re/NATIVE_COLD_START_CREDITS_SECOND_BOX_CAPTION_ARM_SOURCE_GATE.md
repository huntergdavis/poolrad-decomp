# Cold-start credits second box: conditional caption-arm source gate

This source-only gate covers `RES_03F1:+0333..+035C`, the nonzero-caption
fallthrough after the [second-box return branch](NATIVE_COLD_START_CREDITS_SECOND_BOX_RETURN_FRAME_SOURCE_GATE.md).
It stops **before** the optional resident printer far call at `+035C`.
The [second-box U17 caller](NATIVE_COLD_START_CREDITS_SECOND_BOX_SOURCE_GATE.md)
supplies a zero-length Pascal caption, so this note does not claim that
the fallthrough executes for that call. The
[first-printer source gate](NATIVE_COLD_START_CREDITS_FIRST_PRINTER_SOURCE_GATE.md)
owns the separate U17 printer caller; this note does not enter it.
All bytes below were rechecked against ignored DOS v1.3 originals and
retained U17-entry RAM. Bounded `deja` for the border and caption
continuation failed because its local `index.db.lock` is read-only.
PM, p5, and p9 cleared this exact address slice and unique file path.
No emulator was started under the heavy-native hold.

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

The 41-byte arm is original `START.EXE` `[0x04285,0x042AE)`,
SHA-256 `f0ecf830fd8b7833fe0db0fd5e6e39df296c942a9c3976ecf5100333e5182737`.
Retained `0C15:+0333..+035C` has the **same** SHA-256 and all 41 bytes
match. The excluded five-byte optional printer call starts at
`+035C`, original `0x0709:0x0626` and relocated `0F2D:0626`; its
segment relocation and any printer behavior are outside this gate.

| Wrapper offset | Source operation on the nonzero arm |
| --- | --- |
| `+0333..+033A` | Read copied Pascal length from `SS:[BP-0x29]`, clear AH, and save the unsigned length in CX. |
| `+033A..+034E` | Sign-extend low-byte right and left coordinates from `SS:[BP+0x10]` and `SS:[BP+0x14]`; compute `(left + right - length) / 2` with signed 16-bit `IDIV 2`. |
| `+034E..+0357` | Push computed AX column, then push AX with row low byte from `SS:[BP+0x12]` and color low byte from `SS:[BP+0x0A]`. |
| `+0357..+035C` | Form and push far pointer `SS:BP-0x29` to the copied caption; stop before the optional far call. |

For the merged second-box caller, left/right/row/color low bytes are
`1/38/5/0`. Only the computed column push has an explicitly calculated
full AX value on this arm; the later `MOV AL` instructions leave AH as it
was, so this gate does not assert full-word row or color arguments.
The length source is zero for the pinned second-box caller. If its
SYSTEM copy returns normally with that zero preserved, the preceding
`JE +0361` skips this entire arm. A nonzero copied length is only a
source-level alternate condition here, not a captured native frame.

This gate adds no optional-printer execution, EGA page or mask,
framebuffer writer, natural timing, C translation, or frame-parity
credit. Its pre-printer stop keeps it disjoint from p9's U17 printer
caller notes and the separate border setup gate.
