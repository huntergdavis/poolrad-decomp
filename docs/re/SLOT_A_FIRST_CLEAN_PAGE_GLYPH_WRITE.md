# Saved slot-A first-view clean page byte: first value-changing writer

## Boundary and prior work

This note isolates one byte outside the first `RES_0464:1B52` flush's dirty
spans. It does not claim a full first-view frame or complete translation of
the resident glyph drawer. Before debugging, `deja "Poolrad slot A first view
clean A000 byte write watch after 0C88 1B52 flush before 3582 3387"` found
no indexed session. The experiment reuses the
[accepted first-flush indexed page audit](SLOT_A_U40C_DIRTY_INDEXED_VIEW.md),
the [font block and glyph ABI recovery](FONT_8X8D_201.md), and the
[native BPM watch protocol](SLOT_A_ONE_UP_NATIVE_WRITE_WATCH.md).

The owner-supplied DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Its expanded resident glyph drawer image `[0x0726F,0x0767A)` is
1,035 bytes, SHA-256
`8d71dd4c0162462f99aef0fbbd4cfee472702f330bca1e07b50f192ffd972aef`.
The same runtime span at `0F2D:01DF..05E9` in the paused run hashes to
`05468fbf187f03220b52df161cea902636eb43d908da6ea9ae19543a2855b26e`.
The different hashes reflect loaded segment fixups. The watched store at
`RES_0709:0412` maps to expanded image `0x074A2`.

## Native breakpoint result

The ignored run is
`resources/oracle-runs/slot-a-u40c-clean-writer-p5-001/`. The first
`0C88:1B52` flush entered with `DS=14A0`, `SS:SP=1D16:3E94`; its
immediate return was `2D14:05FB`. Full RAM SHA-256 at those stops:
`3f5651da5266366c8b1f85e9ba861dd673c2529a9ccea3012749d711f48394e7`
and
`3ec8495e1692eb43b6cbcfb4d496e06efad717e0fccdf59fb1876e4fae8b912c`.

The selected `A000:0511` page byte is display row 32, columns 136..143.
It is outside the first-flush dirty columns 24..111 for that row. All four
plane bytes were zero immediately after the first flush. A `BPM A000:0511`
then fired **before** the next `0C88:1B52` entry, reporting the first
value change `00 -> 3F` on plane 0. Post-instruction `CS:IP=0F2D:0415`;
the preceding instruction at `0F2D:0412` is `mov byte ptr es:[di],dl`.
The source at `:0407` reads one glyph row from the far font buffer
`DS:[5E20]`; `:040A..0411` sets `ES` from page segment `DS:[4A16]`
and `DI` from the glyph drawer's byte offset. At the watch,
`ES=A000`, `DI=0511`, `DL=3F`, `DS:[4A1E]=1` (EGA mode), and
`DS:[4A16]=A000`. The watch RAM SHA-256 is
`dd9a7368aeebf810b7b63d609825ff5846fbc6e43d8d354cb4e1570b6aeda6e2`.

The `SS:BP=1D16:3D8E` frame has column `0x11`, row `0x04`, color
`0x0F`, count 1, character `0x54`, upcase flag 1. The effective glyph
index is `0x14` and glyph scanline is 0. The live 1,416-byte font buffer
at `DS:[5E20]` hashes to the documented unpacked block-201 SHA-256
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`;
the selected font row supplies `0x3F`. The return frame is
`0F2D:0694`, immediately after the string printer's call at
`RES_0709:0691` to `:01DF`. This proves the reached writer and its
immediate caller, without identifying the earlier overlay print request.

At the later flush entry, the selected sampled plane was still `0x3F`.
At first-view return `3582:3387`, all four plane bytes were `0x3F`.
Each zero one-byte capture hashes to
`6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d`;
each `0x3F` capture hashes to
`8a8de823d5ed3e12746a62ef169bcf372be0ca44f0a1236abc35df05d96928e1`.
The watch was armed at the first flush's immediate return and reports
value-changing writes. It cannot detect same-value stores; nor does this
single address identify all 1,404 changed clean plane bytes.

## Bounded C dependency

`pr_slot_a_glyph_plane_byte` composes the merged
`pr_font201_glyph_index` and `pr_font201_glyph_row` with the four EGA
color-plane gates at `RES_0709:03E4..0412`. It accepts one glyph
scanline and plane. Synthetic tests cover upcase versus raw glyph index,
selected and cleared color planes, another scanline, and invalid row,
plane, font and result pointer. The private checker passes the live,
hash-verified font buffer and frame arguments through this C helper:
**4/4** predicted plane bytes equal the native first-view-return bytes.
This is a one-byte dependency model of the reached EGA path, not full
`RES_0709:01DF` parity or strict full-C credit.

```sh
cmake -S . -B resources/build-clean-glyph-byte \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-clean-glyph-byte --target \
  poolrad_slot_a_glyph_plane_byte_tests poolrad_slot_a_glyph_plane_probe \
  poolrad_font_8x8d_201_tests
ctest --test-dir resources/build-clean-glyph-byte \
  -R '^(poolrad_slot_a_glyph_plane_byte_tests|poolrad_font_8x8d_201_tests)$' \
  --output-on-failure
python3 tools/replay/slot_a_clean_glyph_byte_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --run resources/oracle-runs/slot-a-u40c-clean-writer-p5-001 \
  --probe resources/build-clean-glyph-byte/poolrad_slot_a_glyph_plane_probe
```

All original DOS files, RAM, font bytes and page captures remain under ignored
`resources/`.
