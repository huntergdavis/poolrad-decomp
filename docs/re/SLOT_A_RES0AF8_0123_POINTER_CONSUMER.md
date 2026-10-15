# Loaded RES_0AF8:0123 far-pointer consumer

After the joined stores at `131C:011B`, loaded `131C:0123`
consumes the four-byte far pointer at `DS:4418`.
This audit bounds the next ten bytes through the `JE` at `131C:012B`.
The loaded start is physical `0x132E3`; original `0AF8:0123` is
physical `0xB0A3`; START file source is `0x0AD99`.

`deja "Poolrad 131C:0123 DS:4418 far pointer consumer 131C:0608 DS:4426 store source"`
found no indexed session. PM assigned this source-only follow-on in
inbox i5860. PM's i5847 clearance for the containing `011B`
successor and a tracked-file screen found no overlap.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START consumer | `0x0AD99..0x0ADA2`, 10 bytes | `90933d306e560eec30ef91dcdb77d5895c6f68a815df596d43335a70797651c5` |
| Loaded consumer | `0x132E3..0x132EC`, 10 bytes | `90933d306e560eec30ef91dcdb77d5895c6f68a815df596d43335a70797651c5` |
| `LES` input | START `0x0AD99..0x0AD9C`, loaded `0x132E3..0x132E6`, 4 bytes | `e546db32a2cc6329368ab7a74f1dfe0641fd39c0bfa0a5cd1c9ee36ea918a8a9` |
| Decision tail | START `0x0AD9D..0x0ADA2`, loaded `0x132E7..0x132EC`, 6 bytes | `404564f1a18d7f24cd9863f78f18d7db28cb08bd41d815f976c2c9317eca490b` |

The ten bytes have one START occurrence and match loaded RAM. Four
instructions load the pointer offset into BX and its segment into ES,
copy ES to AX, OR BX into AX, and jump to `131C:0140` if both pointer
words are zero. Otherwise control falls through to `131C:012D`.
The `0140` call site and `012D` stack transfer need separate effect
evidence.

The pointer value is an explicit unsupported input. Without the four
bytes at `DS:4418..441B`, this gate stops before `LES`; it supplies
no branch outcome. The preceding `011B` word stores are source facts,
not observed writes. Even with a supplied pointer, this audit stops
at the two successor boundaries, before either target's effects.
Original and loaded physical addresses differ by `0x08240`;
original and START file addresses differ by `0x030A`.

Reproduce with
`python3 tools/replay/slot_a_res0af8_0123_pointer_consumer_audit.py`.
No native reach, C parity, or full-C count is claimed. Original media
and RAM stay ignored under `resources/`.
