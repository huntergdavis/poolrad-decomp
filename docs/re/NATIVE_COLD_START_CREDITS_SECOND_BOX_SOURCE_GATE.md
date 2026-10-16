# Cold-start credits second box: bounded source gate

This source-only gate covers U17 `+0232..+0258`, from the first credits
box-call return through the second box-call return. It ends exactly where
the [first credits printer](NATIVE_COLD_START_CREDITS_FIRST_PRINTER_SOURCE_GATE.md)
starts. The [first-box source gate](NATIVE_COLD_START_CREDITS_WRITER_OWNERSHIP_GATE.md)
supplies the preceding return and the established `RES_03F1:+02E8` helper
ABI; the [credits source contract](COLD_START_CREDITS_SOURCE_CONTRACT.md)
supplies the box layout. All spans here were rechecked against ignored DOS
v1.3 originals and retained U17-entry RAM. Bounded `deja` for this second
box failed because its local `index.db.lock` is read-only. PM cleared this
span against p9's printer callers and p5's U19 row-14 work. No emulator was
started under the heavy-native hold.

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

The 38-byte U17 caller span `+0232..+0258` maps to original `GAME.OVR`
`[0x00306,0x0032C)`, SHA-256
`47ebaf2cd24308dcc69f0a797fae6c3f5cb97e0d6aa3a8d851ed344a8de82f70`.
Loaded `3909:+0232..+0258` hashes
`abd726da0e6e73d791127a6231b1882d4f5a6064847045a96406f06c69b0dfe8`.
Only the four far-call segment bytes at `+0251,+0252,+0256,+0257`
differ. The original target segment words are `0AF8` and `03F1`; the
retained RAM words are `131C` and `0C15`. These are relocation values in
an entry snapshot, not captured call-site registers.

| U17 offset | Source operation and static successor |
| --- | --- |
| `+0232..+0244` | Set AL and push AX six times, producing low-byte rectangle/style operands `(1,5,38,23,0,0)`; the upper AX bytes are not reset by these instructions. |
| `+0244..+0249` | Form and push far destination `SS:BP-1`. |
| `+0249..+024E` | Push `CS:+004A` as the inline Pascal source pointer. |
| `+024E..+0253` | Far-call SYSTEM `0AF8:0B0B`, statically returning at `+0253`. Loaded target segment is `131C`. |
| `+0253..+0258` | Far-call resident `03F1:02E8`, statically returning at `+0258`. Loaded target segment is `0C15`. |

The source byte at `CS:+004A` maps to original `GAME.OVR`
`[0x0011E,0x0011F)` and retained `3909:+004A..+004B`. Both equal a
zero-length Pascal caption byte, SHA-256
`6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d`.
The second box's inclusive cell rectangle is `(1,5)..(38,23)` with
zero style operands. Its source-site arguments differ from the first box's
`(1,1)..(38,3)` even though both call the same resident helper.

| Call instruction | Original `GAME.OVR` span and SHA-256 | Loaded SHA-256 |
| --- | --- | --- |
| SYSTEM copy `+024E..+0253` | `[0x00322,0x00327)` `7780859e922a704a7253291b792982fced470c34e06c47f1daf92468f6774bae` | `1b227ae15a60114aee8a68adef67dd8cad844a7d284497c578ca3160566b5a9b` |
| Box helper `+0253..+0258` | `[0x00327,0x0032C)` `f1c7615b48a308bf93bd3b12bd137453841816d9cf8d070481e87cb40399445b` | `e1d48e6e44315e0d78c8744ae728bd79dc9d58b614fdf111acbf805ada0fd834` |

Those call-instruction hashes equal the first box's corresponding hashes
because their target offsets and relocation values are the same. The
caller operands above distinguish the second box. The next instruction
at `+0258` begins p9's separately owned first-printer setup.

This gate records source operands, relocated targets, and return offsets.
It does not show that either call ran in a native credits pass, identify a
first framebuffer writer or visible page, measure natural title-to-credits
timing, or add C translation or frame-parity credit.
