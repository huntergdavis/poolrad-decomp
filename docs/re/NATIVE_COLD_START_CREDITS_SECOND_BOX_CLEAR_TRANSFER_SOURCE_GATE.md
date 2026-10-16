# Cold-start credits second box: nested clear-transfer source gate

This source-only gate covers `RES_03F1:+0301..+0315` while the
[second U17 credits box](NATIVE_COLD_START_CREDITS_SECOND_BOX_SOURCE_GATE.md)
is the caller. It records the four rectangle operands forwarded to the
nested `+0137` helper and the static call boundary. The
[first-box source audit](NATIVE_COLD_START_CREDITS_WRITER_OWNERSHIP_GATE.md)
supplies the established helper and rectangle-fill map; the separate
[second-box return gate](NATIVE_COLD_START_CREDITS_SECOND_BOX_RETURN_FRAME_SOURCE_GATE.md)
owns its later zero-caption branch and epilogue. All bytes below were
rechecked against ignored DOS v1.3 originals and retained U17-entry RAM.
Bounded `deja` for this nested transfer failed because its local
`index.db.lock` is read-only. PM, p5, and p9 cleared this exact address
slice and unique file path. No emulator was started under the heavy-native
hold.

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

The 20-byte transfer is original `START.EXE` `[0x04253,0x04267)`,
SHA-256 `a5c60105d18b16c43fd6c0f7ea2b0240d2121f8b6fe367095a10d62984d23d61`.
Retained `0C15:+0301..+0315` has the **same** SHA-256 and all 20 bytes
match. Its final four-byte `PUSH CS; CALL +0137` at
`+0311..+0315` maps to `[0x04263,0x04267)` and has identical
original/loaded SHA-256
`3c18135cc6d768bf60ff388e1963eb64e11b4a47fe8275279a6f9eb5390b3c3a`.
The near call's target is within the same resident segment; its static
return offset, if the helper completes, is `+0315`.

| Wrapper offset | Source operation | Second-box low-byte operand |
| --- | --- | ---: |
| `+0301..+0305` | Load AL from `SS:[BP+0x14]`; push AX. | left `1` |
| `+0305..+0309` | Load AL from `SS:[BP+0x12]`; push AX. | top `5` |
| `+0309..+030D` | Load AL from `SS:[BP+0x10]`; push AX. | right `38` |
| `+030D..+0311` | Load AL from `SS:[BP+0x0E]`; push AX. | bottom `23` |
| `+0311..+0315` | Push CS, then near-call `+0137`. | return `+0315` |

Those four values come from the merged U17 `+0232..+0258` caller's
inclusive `(1,5)..(38,23)` rectangle. Each instruction overwrites AL
only, so this note pins the forwarded **low bytes**, not the upper AX
bytes or a captured CPU stack. The nested `+0137` source contains the
separate EGA rectangle calls mapped by the first-box audit; this call
site is a static candidate transfer into that routine for the second
box. It ends before the border-call setup and before the later caption
branch, and does not enter p9's U17 first-printer caller at `+0258`.

There is no native stop at this second-box transfer, no measured EGA
mask or page, and no attributed framebuffer write. This source gate
adds no first-writer, natural-timing, C translation, or frame-parity
credit.
