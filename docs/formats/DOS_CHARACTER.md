# DOS v1.3 character record

This is a read-only format layer for one already loaded `CHRDAT*.SAV` file.
The C descriptor borrows the caller's 285 bytes; the caller must keep them
alive and unchanged. It provides bounded raw spans and a span for the verified
length-prefixed name bytes. It does not decode the DOS character encoding or
interpret stats, sidecar contents, or unknown bytes.

## Field ledger

The evidence below is for the supplied DOS v1.3 build. A *verified* shape is
byte-checked in all supplied records; it does not imply that its meaning has
been recovered from the DOS engine.

| DOS record offset | Width | Meaning / byte order | Read/write evidence | Confidence / API |
| ---: | ---: | --- | --- | --- |
| `0..284` | 285 | Exact complete record frame | GAME.OVR unit `0x6f` reads `0x011d` bytes at `0xc3a..c44` and writes `0x011d` bytes at `0x8b3..8c6`; all 12 supplied files have this length | High; `pr_dos_character_open` requires exactly 285 bytes |
| `0` | 1 | Length of the internal Pascal character-name byte string; unsigned byte, no byte order | Record base is passed to string composition at unit `0x6f:0x7b7` and `0x6f:0x215d`; all 12 records have length 6–14, followed by printable bytes and zero padding through offset 15 | High for length-prefix meaning; `pr_dos_character_name_span` bounds it to 15 |
| `1..15` | at most 15 | Internal character-name bytes, encoding left raw | The same string uses place the record base in pathname composition; all supplied records have exactly `data[0]` printable bytes followed by zero padding through offset 15 | High for occupied byte span, observational for the 15-byte capacity; accessor returns bytes `1..1+length` without conversion |
| `0xbf` | 1 | In-memory party slot, assigned from the first free index `0..7`; no byte order | Unit `0x6f:0x144c..1565` initializes and chooses the slot; `0x6f:0x19da..19e3` calls this after disk load. The whole 285-byte object is saved, but the slot is recomputed during load | High for runtime use, provisional for persisted meaning; raw only |
| `0x7f..0x82`, `0xc8..0xcb`, `0x104..0x10b` | 4+4+8 | In-memory far-pointer slots, not portable file offsets | Unit `0x6f:0xc54..c8a` clears them immediately after the 285-byte disk read, before sidecars are loaded | High for runtime pointer shape; raw only |
| `284` | 1 | Meaning unknown | Across the 12 records, observed values are 6, 9, or 12 (2, 6, and 4 records respectively); no instruction trace ties this byte to a class or stat | Low; raw only |
| All other offsets | — | Unknown fields and boundaries | No DOS instruction trace or controlled edit/load observation yet establishes their meanings | Raw spans only |

The [Macintosh v1.1 save specification](../prior-research/macmaps/SAVE_FORMAT.md)
describes a 302-byte character record in a resource fork. Its field names and
offsets are research leads, not DOS v1.3 facts. A byte-pattern scan of the
supplied DOS executables finds 285 as a little-endian word in multiple places;
the trace below identifies the relevant I/O calls. This session ran
`deja "DOS CHRDAT SAV load write path character record offsets"` and
`deja "CHRDAT"`; neither supplied a prior instruction trace. The earlier
[DOS audit](../FABLE_AUDIT.md) and this document's framing/sidecar results
were reused. The Macintosh research remains a lead only.

## DOS load, save, and name trace

All addresses below are offsets within decoded GAME.OVR unit `0x6f` code
at file offset `0x1dfe6` (unit code SHA-256
`30da2d2a28149942431395bf59511b55a9eab4d8f728bff625c5a22d81638882`).
The offset-to-function mapping uses the [overlay inventory](../re/overlay_inventory.md).
The far-call segment operands at the cited runtime I/O and string-composition
sites appear in that unit's fixup table. Runtime helper identities are inferred
from arguments and use, pending lane A review of the overlay mapping.

| Path | Instruction evidence | Result |
| --- | --- | --- |
| Load wrapper, `0x1602` (file `0x1f5e8`) | At `0x19b1`, allocates `0x011d` bytes, pushes the resulting far pointer and an assembled basename, calls `0xaec` at `0x19d7`, then calls list insertion `0x1446` at `0x19e3` | One full record is loaded into a heap object and linked into the party list |
| Load routine, `0xaec` (file `0x1ead2`) | The selected suffix at `0xad8` is the Pascal literal `.sav`; at `0xc3a..c44` the routine passes its `[bp+0xa]` far-pointer object base and length `0x011d` to resident `0xAF8:0x14FB`. At `0xc54..c8a` it clears far-pointer slots, then loads `0x3f`- and `9`-byte sidecar units | The 285-byte read precedes runtime pointer repair and sidecar reads |
| Save wrapper, `0x1d58` (file `0x1fd3e`) | At `0x2113` it starts a basename with the Pascal `CHRDAT` literal at `0x1d40`; at `0x213e` it calls `0x69e` with that basename and the current record pointer | The disk wrapper is `CHRDAT`-prefixed; the internal name need not equal the file stem |
| Save routine, `0x69e` (file `0x1e684`) | At `0x7b7` the `[bp+0xa]` record base is passed as a Pascal-string source in pathname composition; the suffix at `0x67b` is `.sav`. At `0x8b3..8c6` the routine passes that same record base and `0x011d` to resident `0xAF8:0x1502`, then writes `0x3f`- and `9`-byte sidecar units | The full object is written to the `.sav` file; no field-aware serializer is shown |
| Further name use | At `0x215d` the save wrapper passes the record base to the same string-copy call used with literal Pascal strings, while building companion paths | Offset 0 is read as the Pascal name length, followed by name bytes; no C-string terminator is required |

The 12 private stems all begin with `CHRDAT`; none equals or contains its
record's internal name. This disproves the tempting hypothesis that the file
stem is the character-name field. The previous hypothesis that offsets
`0..15` were an unknown NUL-padded prefix is replaced by the length-prefixed
use. Offset `284` as class group remains unverified. The next targeted DOS
oracle experiment is to create or rename one character, save it, and compare
only changed offsets and the game's displayed name across the before/after
records. A separate party reorder can test whether offset `0xbf` is persisted
as a meaningful slot or always regenerated. No such oracle result is claimed.

The address and size trace can be reproduced without publishing executable
bytes. The extraction writes only to `/tmp`; do not add that file to Git:

```sh
python3 tools/overlay/overlay_inventory.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --json /tmp/poolrad-character-units.json
python3 - <<'PY'
import json
from pathlib import Path
units = json.loads(Path('/tmp/poolrad-character-units.json').read_text())['units']
unit = next(u for u in units if u['stub_para'] == 0x6f)
data = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()
start = unit['ovr_offset']
Path('/tmp/poolrad-character-unit.bin').write_bytes(data[start:start + unit['code_size']])
for rel in (0x7c7, 0x8c6, 0xc44, 0x216e):
    assert rel + 3 in unit['fixups']
print('unit', hex(unit['stub_para']), 'file', hex(start), 'fixups_checked=4')
PY
objdump -D -b binary -mi8086 -M intel --no-show-raw-insn \
  --start-address=0x69e --stop-address=0xaec /tmp/poolrad-character-unit.bin
objdump -D -b binary -mi8086 -M intel --no-show-raw-insn \
  --start-address=0xaec --stop-address=0xec4 /tmp/poolrad-character-unit.bin
```

`pr_dos_character_raw_span` returns source and exclusive end offsets after
checking `offset <= 285` and `length <= 285 - offset`. An empty range at offset
285 is valid. It returns no pointer past the borrowed data and never copies or
modifies save bytes. Invalid requests leave the output descriptor unchanged.
`pr_dos_character_name_span` returns offsets `1..1+data[0]` when the length
byte is at most 15. It does not require ASCII, padding, or a NUL terminator;
invalid lengths leave the output descriptor unchanged. The 15-byte cap is a
conservative boundary supported by the supplied corpus, not a claim about
every possible DOS name.

## Sidecar size observations

All supplied `.ITM` lengths are divisible by 63 and all supplied `.SPC`
lengths by 9. These are measured size units, not yet verified item or spell
field layouts. The audit reads no sidecar content. Two character records have
no `.SPC` sidecar in this corpus; a missing spell sidecar is not treated as a
format error. Sidecars that are supplied must pair with a `.SAV` stem.

## Tests and private local audit

```sh
cmake -S . -B /tmp/poolrad-dos-character-semantics-build -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-dos-character-semantics-build
ctest --test-dir /tmp/poolrad-dos-character-semantics-build --output-on-failure
/tmp/poolrad-dos-character-semantics-build/poolrad_dos_character_audit resources/DOS/POOLRAD/CHRDAT*.SAV resources/DOS/POOLRAD/CHRDAT*.ITM resources/DOS/POOLRAD/CHRDAT*.SPC
```

Synthetic tests cover exact framing, undersized and oversized inputs,
one-past empty spans, last-byte spans, integer overflow candidates, invalid
descriptors, empty and full-capacity names, invalid name lengths, and unchanged
outputs after errors. On 2026-10-10, the strict
build passed all six CTest targets. The audit of the ignored owner-supplied
files reported only aggregate sizes and counts:

```text
sav_files=12 sav_bytes=3420 name_min_length=6 name_max_length=14 name_printable_frames=12 name_zero_padded_frames=12 itm_files=12 itm_bytes=1764 itm_size_units=28 spc_files=10 spc_bytes=198 spc_size_units=22 item_pairs=12 spell_pairs=10 failures=0
```

The 12 `.ITM` sizes were two at 63 bytes, four at 126, and six at 189. The
ten `.SPC` sizes were four at 9 bytes, three at 18, one at 27, one at 36, and
one at 45. The target executable SHA-256 hashes were:

```text
START.EXE  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd
GAME.OVR  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614
```

No character names, save bytes, sidecar contents, or original files are part
of the repository or audit output. Other semantic accessors require further
executable tracing or controlled save/load differences.
