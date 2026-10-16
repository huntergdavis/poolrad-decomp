# Cold-start credits second box: first border-corner source gate

This source-only gate follows the [second-box border setup](NATIVE_COLD_START_CREDITS_SECOND_BOX_BORDER_SETUP_SOURCE_GATE.md)
into `RES_03F1:+0000..+003D`, ending at the first overlay corner call's
static return offset. The [first-box border audit](NATIVE_COLD_START_CREDITS_FIRST_BORDER_SOURCE_GATE.md)
maps the same helper and the later candidate display path. This note
adds the second box's distinct source operands; it does not attribute a
pixel or an executed overlay call to a native second-box pass.

The ignored DOS v1.3 `START.EXE` is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Retained U17-entry RAM is 655,360 bytes, SHA-256
`11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6`.
Bounded `deja` failed because its local `index.db.lock` is read-only.
The first-box audit supplied the helper mapping; p5 and PM cleared this
second-box-specific slice and unique note path. No emulator was started
under the host-heavy/native hold.

| Slice | Bytes | SHA-256 | Call target at `+0038` |
| --- | ---: | --- | --- |
| Original `START.EXE` `[0x03F52,0x03F8F)` | 61 | `fedebf7269bd9ab787380a12d05ce11e528c78117099aaa9ce4c57dce3df32a2` | `0428:002A` |
| Retained `0C15:+0000..+003D` | 61 | `6de83e96624ce0265e3db0f377f020c9f11bd979ec8aedf51640cd1231e67f19` | `0C4C:002A` |

Only the two segment bytes at `+003B,+003C` differ between the
original and loaded slices. The second-box caller passes the low-byte
rectangle `(left,top,right,bottom)=(1,5,38,23)` and style zero to the
wrapper. Its border setup forwards those low bytes to this helper;
the source does not establish a captured entry stack.

| Helper offset | Decoded source effect for those low bytes |
| --- | --- |
| `+0000..+0006` | Save BP, establish a frame, and allocate four local bytes. |
| `+0006..+0012` | Decrement left and top bytes, increment right and bottom bytes: expanded low-byte rectangle `(0,4)..(39,24)`. |
| `+0012..+0023` | Zero-extend style and calculate `0x0114 + 3*style`; save first-corner item `0x0114` at `SS:[BP-3]`. |
| `+0023..+0038` | Push first-corner X, Y, item, and two zero low-byte flags. |
| `+0038..+003D` | Far-call overlay stub `0428:002A`, relocated to `0C4C:002A`; source return is `+003D` if it returns. |

The first-corner push sequence preserves AH from item `0x0114` when it
loads the X, Y and flag low bytes into AL. For this style-zero source
case, the **full pushed AX words**, in push order, are `0x0100`
(X low byte 0), `0x0104` (Y low byte 4), `0x0114` (item), `0x0100`
and `0x0100` (both flags low byte 0). These are calculations from
instructions and caller low-byte operands, not a captured frame or a
claim about how the overlay consumes their upper bytes.

The first-box audit maps the overlay's possible direct-copy and display
successors. Whether this second-box corner call runs, takes that branch,
or changes a visible pixel remains unmeasured. This gate adds no first
writer, EGA page or mask, natural timing, C translation, or frame-parity
credit.
