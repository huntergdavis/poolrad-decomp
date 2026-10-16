# Cold-start credits second box: conditional caption-printer prefix

This source-only gate links the [second-box conditional caption arm](NATIVE_COLD_START_CREDITS_SECOND_BOX_CAPTION_ARM_SOURCE_GATE.md)
and [optional printer call](NATIVE_COLD_START_CREDITS_SECOND_BOX_OPTIONAL_PRINTER_CALL_SOURCE_GATE.md)
to the shared `RES_0709:+0626..+0641` printer prefix. The
[merged printer prefix gate](RESIDENT_0709_0626_FIRST_COPY_GATE.md)
already maps and models that code for other callers. The distinct fact
here is the second-box wrapper's **conditional caller source**: its
stack-local copied caption, not U17's subsequent inline first-printer
string. The [U17 second-box caller](NATIVE_COLD_START_CREDITS_SECOND_BOX_SOURCE_GATE.md)
supplies a zero-length Pascal caption, so this note does not claim the
optional printer is reached in a native second-box pass.

The ignored DOS v1.3 `START.EXE` is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Retained U17-entry RAM is 655,360 bytes, SHA-256
`11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6`.
Bounded `deja` failed because its local `index.db.lock` is read-only.
PM and p9 cleared this distinct caller-context note and unique path.
No emulator was started under the host-heavy/native hold.

| Prefix slice | Bytes | SHA-256 | External call at `+063C` |
| --- | ---: | --- | --- |
| Original `START.EXE` `[0x076EF,0x0770A)` | 27 | `372493c3602f9a20a394587515421dd79b7fdb8932a2c728bd4ec390d8647a4b` | `0AF8:0B25` |
| Retained `0F2D:+0626..+0641` | 27 | `3b42c64fd0281d6fe846b3f24024dfddf6b2d52bc0ea3844b4168b628e6a0085` | Relocated `131C:0B25` |

Only the two far-call segment bytes at `+063F,+0640` differ. The
prefix saves BP, allocates `0x104` bytes, loads the source far pointer
from its caller's `[BP+6]`, and pushes that pointer, destination
`SS:BP-0x100`, and cap `0x00FF` for the SYSTEM copy. Its source
successor is `+0641` if the copy returns normally. It has no local
branch before that call. The shared C transition already covers this
prefix; this note adds no new C behavior or parity credit.

At `RES_03F1:+0331`, a zero copied Pascal length branches around the
caption arm and printer call to `+0361`. Only a **nonzero** copied
length `L` enters that alternate source path. For the second-box
rectangle's left/right low bytes `1/38`, the arm computes a signed
16-bit column by dividing `39-L` by 2, pushes row low byte `5` and
color low byte `0`, then pushes far pointer `SS:BP-0x29` to its own
copied caption. The optional `+035C` far call would pass that pointer
to this printer prefix, whose `[BP+6]` reads it. The later
[U17 first-printer call](NATIVE_COLD_START_CREDITS_FIRST_PRINTER_SOURCE_GATE.md)
instead passes a separate local copied from its own inline source;
these are different source routes into the shared prefix.

The original second-box source length is zero. Whether the preceding
copies return, what a hypothetical nonzero copied length would be, and
the printer's later guards, glyphs, page and framebuffer effects are
not observed here. This gate adds no natural timing, first writer,
full C translation, or frame-parity credit.
