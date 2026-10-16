# Cold-start credits second box: static return-frame gate

This source-only gate follows the [second-box caller](NATIVE_COLD_START_CREDITS_SECOND_BOX_SOURCE_GATE.md)
through two disjoint instruction slices in `RES_03F1:+02E8`: the copied
caption-length branch at `+032D..+0333` and the epilogue at
`+0361..+0367`. The [first-box helper audit](NATIVE_COLD_START_CREDITS_WRITER_OWNERSHIP_GATE.md)
pins the full wrapper and its nested clear calls. The
[codeword box source map](COLD_START_CODEWORD_BOX_SOURCE.md) supplied the
empty-caption branch as a lead; all slices here were rechecked against
ignored DOS v1.3 `START.EXE` and retained U17-entry RAM. Bounded `deja`
for this return route failed because its local `index.db.lock` is
read-only. PM, p5, and p9 cleared this exact source-only scope and unique
note path. No emulator was started under the heavy-native hold.

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

The full `RES_03F1:+02E8..+0367` wrapper is 127 bytes at original
`START.EXE` `[0x0423A,0x042B9)`, SHA-256
`71dfae69a38bb9aadad319291d7218cd65523ba14162fb45e5e699ca2d6816d1`.
Retained `0C15:+02E8..+0367` hashes
`c9e254e0f313763bbdafaba8ee76d60535e6835322468f5c85257b121badac1c`.
The two selected six-byte slices are identical in original and loaded RAM:

| Resident slice | Original `START.EXE` | Bytes | Original / loaded SHA-256 | Source effect |
| --- | --- | ---: | --- | --- |
| `+032D..+0333` | `[0x0427F,0x04285)` | 6 | `a3ab08f2cc6323de2d62917daa71c8fce4620b8aa15c73d9e53192ea83d5c554` | Compare copied Pascal length at `SS:[BP-0x29]` with zero; `JE +0361`. |
| `+0361..+0367` | `[0x042B3,0x042B9)` | 6 | `3ac97302023de372b68e01b4a0780e43d4eab60646b5c4e57dcaa96080fc67ee` | `MOV SP,BP; POP BP; RETF 0x10`. |

The merged caller note pins U17's second-box far call at `+0253..+0258`
and the zero-length Pascal source byte at `CS:+004A`. The wrapper copies
the caller's caption into its `SS:BP-0x29` local buffer through SYSTEM
`+0B25` before the branch. **If** that copy returns normally with the
zero length preserved, `JE +0361` skips the optional caption-printer
call at `+035C`. The epilogue restores the entry stack frame and
`RETF 0x10` removes the caller's 16 argument bytes: six pushed AX words
and the four-byte far pointer to its local caption. Its static far
return is U17 `3909:+0258`, the start of p9's separately owned
first-printer setup. The branch target and return offset are source
control-flow boundaries, not native stops or captured stack values.

The skipped middle of the wrapper contains other source paths and a
relocated optional-printer call; this gate assigns no runtime outcome to
them. It adds no visible-page, first-writer, natural-timing, C translation,
or frame-parity claim. The separate pre-return `+0301..+0315` nested
clear-helper transfer is reserved for its own source note and branch.
