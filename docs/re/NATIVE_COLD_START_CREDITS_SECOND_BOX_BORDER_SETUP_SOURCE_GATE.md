# Cold-start credits second box: border setup source gate

This source-only gate covers `RES_03F1:+0315..+032D` after the
[second-box nested clear transfer](NATIVE_COLD_START_CREDITS_SECOND_BOX_CLEAR_TRANSFER_SOURCE_GATE.md).
It records five low-byte operands forwarded to the local border helper
and the static `+0000` call boundary. The
[second-box U17 caller](NATIVE_COLD_START_CREDITS_SECOND_BOX_SOURCE_GATE.md)
pins the selected rectangle; the
[first-border source audit](NATIVE_COLD_START_CREDITS_FIRST_BORDER_SOURCE_GATE.md)
maps the same helper for the first box. All bytes below were rechecked
against ignored DOS v1.3 originals and retained U17-entry RAM. Bounded
`deja` for this border and caption continuation failed because its local
`index.db.lock` is read-only. PM, p5, and p9 cleared this exact address
slice and unique file path. No emulator was started under the heavy-native
hold.

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

The 24-byte setup is original `START.EXE` `[0x04267,0x0427F)`,
SHA-256 `528522925954c3c5e51abb576640bf42665e1850dda192816e566585617f12a6`.
Retained `0C15:+0315..+032D` has the **same** SHA-256; all 24 bytes
match. Its final four-byte `PUSH CS; CALL +0000` at `+0329..+032D`
maps to `[0x0427B,0x0427F)` and has identical original/loaded
SHA-256 `c339676ccba3b4aa3cf25be0a29ece2f4cd74158c355984f4e34c730cc42f3a1`.
The near call targets the local `RES_03F1:+0000` border helper; its
static return offset, if the helper completes, is `+032D`.

| Wrapper offset | Source operation | Second-box low byte |
| --- | --- | ---: |
| `+0315..+0319` | Load AL from `SS:[BP+0x14]`; push AX. | left `1` |
| `+0319..+031D` | Load AL from `SS:[BP+0x12]`; push AX. | top `5` |
| `+031D..+0321` | Load AL from `SS:[BP+0x10]`; push AX. | right `38` |
| `+0321..+0325` | Load AL from `SS:[BP+0x0E]`; push AX. | bottom `23` |
| `+0325..+0329` | Load AL from `SS:[BP+0x0C]`; push AX. | style `0` |
| `+0329..+032D` | Push CS, then near-call `+0000`. | return `+032D` |

The caller's inclusive cell rectangle is `(1,5)..(38,23)` with style
zero. Each `MOV AL` leaves AH as it was, so the table pins only the
forwarded low bytes, not a captured CPU stack. The first-border audit
maps `+0000` to border-corner and edge calls, but this note does not
attribute those calls or any resulting pixels to a running second-box
pass. The next source boundary at `+032D` belongs to the separate
[zero-caption return gate](NATIVE_COLD_START_CREDITS_SECOND_BOX_RETURN_FRAME_SOURCE_GATE.md).

This gate records original/loaded bytes, caller-derived operands, and
static call/return offsets. It establishes no native execution, EGA page
or mask, first framebuffer writer, natural timing, C translation, or
frame parity.
