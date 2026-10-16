# Cold-start codeword box: source and composition boundary

This is a source-only map of the box drawn by unit `0x19` entry 1 before the
codeword text. It establishes the rectangle-clear and border-tile call chain,
including the number and placement of calls. It does **not** attribute every
visible border pixel or claim a complete C screen.

A bounded `deja "Poolrad U19 codeword border background RES_03F1:02E8 selected
record glyph string source cold start"` query timed out after 20 seconds with
no result. The merged [codeword source](COLD_START_CODEWORD_SOURCE.md),
[resident rectangle model](RES_0709_RECT_FILL_00B7.md), and
[unit `0x40C` entry-2 map](SLOT_A_U40C_FIRST_TILE_PLANE.md) were reused as
leads. The spans and calls below were checked against the ignored DOS media.

## Original-byte route

The pinned DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.

| Original span | Bytes | SHA-256 | Role |
| --- | ---: | --- | --- |
| U19 `+0119..+013F`, file `[0x0AC5,0x0AEB)` | 38 | `42a556be90efe14d010ca2fedb02b7b73756fdca7472ccb3a135a7c89589ef6f` | push six low-byte values; copy empty Pascal caption; call box wrapper |
| `RES_03F1:+02E8..+0367`, image `[0x41F8,0x4277)` | 127 | `71dfae69a38bb9aadad319291d7218cd65523ba14162fb45e5e699ca2d6816d1` | copy caption, call clear and border, skip caption printer when empty |
| `RES_03F1:+0137..+02E8`, image `[0x4047,0x41F8)` | 433 | `1258f9ea251527a82be56c60ecbd1f07e300c994bc0fdbac2a6537369080f5fc` | display-mode branch and two rectangle calls |
| `RES_03F1:+0000..+0137`, image `[0x3F10,0x4047)` | 311 | `fae6cb41423bcd48237e75a08f5eda70642254eac8449373291d57f34120a8a4` | border corners and edge loops |
| U40C entry 2, file `[0x32392,0x324E2)` | 336 | `2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00` | reached *statically* by each border tile call |

U19 pushes six words whose low bytes are `(1,1,38,22,0,0)` and calls
`03F1:02E8` at `+013A`. The wrapper calls its local `+0137` at `+0312`,
then `+0000` at `+032A`. The copied caption has length zero, so its optional
printer is skipped on this input. At the existing pre-input native stop,
`DS:4A1E=1`, consistent with `+0137`'s EGA branch; there is no native stop
inside either box callee in this audit.

The `+0137` EGA branch calls `0709:00B7` at `+01F1` and `+020C`. The
decoded low-byte frames select color zero on pages zero and one, each over
inclusive columns `1..38`, rows `1..22`. The merged rectangle C model covers
the bounded callee semantics separately. No page pair was captured around
these two cold-start calls, so their value changes are not counted here.

The `+0000` routine expands the box to columns `0..39`, rows `0..23`.
Its style-zero three-byte source table is at `DS:0114`; the same-run RAM
slice hashes
`684f280b93fa5c3ea9fda9745dc3c5a45e05c6426252be9826659a535190717c`.
Eight static far-call instructions address overlay stub `0428:002A`, mapped
to unit `0x40C` entry 2. The four corner calls execute once each; two
horizontal calls execute at each of 38 interior columns; two vertical calls
execute at each of 22 interior rows. That is **4 + 76 + 44 = 124** tile calls
for these box arguments. The previously recovered unit-`0x40C` tile model
covers a different saved-view input path; its destination and flags are not
assumed for this codeword box.

## Existing native frame and remaining writer gate

The ignored `native-codeword-read-005` pre-input RAM is 655,360 bytes,
SHA-256
`b4dc553331795b63fb6e8fb6678df59b0a72799f4c386ccdc21ce81b1ec892a3`.
Its paired 640×400 canonical RGB24 image hashes
`5044229ac07b046aaf144113a90e8fec64edcfef47f978d843319991603dabcb`.
The 124 caller-selected outer cells contain **7,936** source pixels, of
which **3,464** are nonblack in that saved image. This is an observed
screen-region count, not a per-instruction attribution.

The next exact gate is an original-byte/native pre/post stop at U19 `+013A`
or inside `03F1:+0000`, with the source-object and EGA page captured before
and after selected `0428:002A` calls. Until then the border tile archive,
constructed object, display transfer, and final border-pixel ownership are
unresolved. No heavy emulator run was started for this note.

The read-only audit in
[`cold_start_codeword_effect_source_audit.py`](../../tools/replay/cold_start_codeword_effect_source_audit.py)
rechecks all image hashes, exact call operands, captured mode/table, static
call counts, and observational ring count. All original bytes, strings,
RAM, and screenshot pixels remain in ignored `resources/`.
