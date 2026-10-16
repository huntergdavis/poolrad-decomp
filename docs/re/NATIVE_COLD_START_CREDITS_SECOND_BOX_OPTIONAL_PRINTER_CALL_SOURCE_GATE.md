# Cold-start credits second box: optional printer-call source gate

This source-only gate covers the five-byte far call at
`RES_03F1:+035C..+0361`. It follows the
[conditional caption arm](NATIVE_COLD_START_CREDITS_SECOND_BOX_CAPTION_ARM_SOURCE_GATE.md)
and ends before the [wrapper epilogue](NATIVE_COLD_START_CREDITS_SECOND_BOX_RETURN_FRAME_SOURCE_GATE.md)
at `+0361`. The [U17 second-box caller](NATIVE_COLD_START_CREDITS_SECOND_BOX_SOURCE_GATE.md)
supplies a zero-length Pascal caption. If the earlier SYSTEM copy returns
with that zero preserved, the `JE +0361` at `+0331` skips this call.
The separate [U17 first-printer caller](NATIVE_COLD_START_CREDITS_FIRST_PRINTER_SOURCE_GATE.md)
is outside this resident slice.

The ignored DOS v1.3 `START.EXE` is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Retained U17-entry RAM is 655,360 bytes, SHA-256
`11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6`.
Bounded `deja` failed because its local `index.db.lock` is read-only.
PM and p9 cleared this disjoint address slice and unique note path.
No emulator was started under the host-heavy/native hold.

| Slice | Bytes | SHA-256 | Decoded source operation |
| --- | ---: | --- | --- |
| Original `START.EXE` `[0x042AE,0x042B3)` | 5 | `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23` | Far call `0709:0626`. |
| Retained `0C15:+035C..+0361` | 5 | `cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585` | Relocated far call `0F2D:0626`. |

The opcode and offset are unchanged; only the call's segment word differs
between the original and loaded bytes. The preceding caption arm pushes a
computed column, row and color words, and a far pointer to its copied local
caption. Its row and color writes replace AL only, so this gate does not
assert their full 16-bit argument values. If the far call returns normally,
the source successor is `RES_03F1:+0361`; this is a control-flow boundary,
not a captured return or a measured printer effect.

This gate adds no optional-printer execution for the zero-caption second
box, no framebuffer writer, EGA page or mask, natural timing, C translation,
or frame-parity credit.
