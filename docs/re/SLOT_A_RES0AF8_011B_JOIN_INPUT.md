# Loaded RES_0AF8:011B joined stores to first input boundary

Two static predecessors reach loaded `131C:011B`: the zero-word
`00FF` branch enters the arithmetic tail at `131C:0114`, while the
equal comparison at `010A` enters the `131C:0112` ES-copy arm. This audit
composes those entries with the first 12 bytes at `011B` and stops
at the far-pointer read from `DS:4418`. Neither entry is known to run.

`deja "Poolrad 131C:011B parent join successor 131C:0604 post indirect call return side"`
found no indexed session. PM assigned this separate source-only gate
in inbox i5835; pF and p5 cleared ownership in i5847.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START join prefix | `0x0AD91..0x0AD9C`, 12 bytes | `38baf501b4f10a9cddf575df137673a0496716ef0d8f526124c89023f78e4ab7` |
| Loaded join prefix | `0x132DB..0x132E6`, 12 bytes | `38baf501b4f10a9cddf575df137673a0496716ef0d8f526124c89023f78e4ab7` |
| Two word stores | START `0x0AD91..0x0AD98`, loaded `0x132DB..0x132E2`, 8 bytes | `751d75c900ef962f95f19f4111459c32cf118af5a3d3a4abd7853db1ebf0249d` |
| First external read | START `0x0AD99..0x0AD9C`, loaded `0x132E3..0x132E6`, 4 bytes | `e546db32a2cc6329368ab7a74f1dfe0641fd39c0bfa0a5cd1c9ee36ea918a8a9` |

The 12-byte source prefix has one START occurrence and matches the
loaded copy exactly. Its three instructions would store incoming CX
at `DS:441E`, incoming BX at `DS:4420`, and then load ES:BX from the
four-byte far pointer at `DS:4418`. On the `0114` entry the stored BX
would be `(BX_in - DS_word[4422] - 0x10) mod 65536`; on the `0112`
entry it would be `(ES - DS_word[4422] - 0x10) mod 65536`. The
`DS:4418` pointer, its segment and offset, and the subsequent branch
are not supplied by this static source. The later `131C:012B` branch
requires a separate supplied pointer state.

The loaded physical address is `0x08240` above the original image,
and the original is `0x030A` above the START file. Reproduce with
`python3 tools/replay/slot_a_res0af8_011b_join_input_audit.py`.
No native reach, C parity, or full-C count is claimed. Original media
and RAM stay ignored under `resources/`.
