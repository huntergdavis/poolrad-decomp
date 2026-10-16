# Cold-start credits second box: caption-copy source gate

This source-only gate covers `RES_03F1:+02E8..+0301`, from the
[second-box U17 call](NATIVE_COLD_START_CREDITS_SECOND_BOX_SOURCE_GATE.md)
through the wrapper's entry prologue and far call to SYSTEM's capped
Pascal-string copy. It stops before the
[nested clear transfer](NATIVE_COLD_START_CREDITS_SECOND_BOX_CLEAR_TRANSFER_SOURCE_GATE.md)
at `+0301`. The [return-frame gate](NATIVE_COLD_START_CREDITS_SECOND_BOX_RETURN_FRAME_SOURCE_GATE.md)
owns the later copied-length branch; the
[runtime string audit](RESIDENT_STRING_0425.md) identifies SYSTEM `+0B25`
as a capped Pascal copy. This note does not claim a captured return or
copied local contents for the second-box call.

The ignored DOS v1.3 `START.EXE` is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Retained U17-entry RAM is 655,360 bytes, SHA-256
`11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6`.
Bounded `deja` failed because its local `index.db.lock` is read-only.
PM's p5/p9 ownership screen cleared this address slice and unique note
path. No emulator was started under the host-heavy/native hold.

| Slice | Bytes | SHA-256 | Source boundary |
| --- | ---: | --- | --- |
| Original `START.EXE` `[0x0423A,0x04253)` | 25 | `7ec657977cc951cdfc0912c960811fc5683bfd72a16618d768360fa6411f7d8f` | Far call `0AF8:0B25`. |
| Retained `0C15:+02E8..+0301` | 25 | `56ec0461ed1ecf0ba825ac3c3179becb98d4c7bf0b45615ea19e283ace795ffb` | Relocated far call `131C:0B25`. |

| Wrapper offset | Decoded source operation |
| --- | --- |
| `+02E8..+02EE` | Save BP, set BP to SP, and allocate `0x2A` stack bytes. |
| `+02EE..+02F3` | Load caller source far pointer from `[BP+6]` into ES:DI, then push ES and DI. |
| `+02F3..+02F8` | Form stack-local destination `SS:BP-0x29`, then push SS and DI. |
| `+02F8..+02FC` | Push AX=`0x0028` as the copy cap. |
| `+02FC..+0301` | Far call the SYSTEM `+0B25` copy helper. |

The original and loaded slices differ only in the far call's segment
word. The pushed argument order is source pointer, destination pointer,
then cap; this is static stack setup, not a measured call frame. If the
SYSTEM call returns normally, the source successor is `+0301`, where
the separately audited rectangle operands are forwarded. The U17
second-box caller supplies a zero-length Pascal caption, but a copied
zero length at the later branch remains conditional on this helper's
normal behavior for that call.

This gate adds no observed clear, border or printer execution, EGA page
or mask, framebuffer writer, natural timing, C translation, or frame
parity credit.
