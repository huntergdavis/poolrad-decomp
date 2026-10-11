# DOS v1.3 character record framing

This is a read-only format layer for one already loaded `CHRDAT*.SAV` file.
The C descriptor borrows the caller's 285 bytes; the caller must keep them
alive and unchanged. It provides bounded raw spans and no save writer, stat
accessor, name decoder, or sidecar-content decoder.

## Field ledger

The evidence below is for the supplied DOS v1.3 build. A *verified* shape is
byte-checked in all supplied records; it does not imply that its meaning has
been recovered from the DOS engine.

| DOS record offset | Width | Observation or meaning | Evidence | Confidence / API |
| ---: | ---: | --- | --- | --- |
| `0..284` | 285 | Exact complete record frame | All 12 supplied `CHRDAT*.SAV` files have this length; the earlier [DOS audit](../FABLE_AUDIT.md) reported the same | High for framing; `pr_dos_character_open` requires exactly 285 bytes |
| `0..15` | 16 | NUL-padded prefix, **meaning unknown** | All 12 prefixes have zero padding; none forms a printable ASCII prefix under a direct byte read or matches a direct prefix in the supplied party-state files | Shape observed only; exposed as raw bytes, not a name |
| `284` | 1 | Raw trailing byte, **meaning unknown** | Across the 12 records, observed values are 6, 9, or 12 (2, 6, and 4 records respectively) | Value distribution verified; class-group interpretation remains provisional and is not an accessor |
| All other offsets | — | Unknown fields and boundaries | No DOS instruction trace or controlled edit/load observation yet establishes their meanings | Raw spans only |

The [Macintosh v1.1 save specification](../prior-research/macmaps/SAVE_FORMAT.md)
describes a 302-byte character record in a resource fork. Its field names and
offsets are research leads, not DOS v1.3 facts. A byte-pattern scan of the
supplied DOS executables finds 285 as a little-endian word in multiple places,
but does not tie any one occurrence to a character field; it was not used as
semantic proof. The exact `deja` DOS character query returned no match. A
broader `deja "DOS character record"` recalled the prior DOS audit and the
Macintosh research; only the DOS size observations were reused here.

`pr_dos_character_raw_span` returns source and exclusive end offsets after
checking `offset <= 285` and `length <= 285 - offset`. An empty range at offset
285 is valid. It returns no pointer past the borrowed data and never copies or
modifies save bytes. Invalid requests leave the output descriptor unchanged.

## Sidecar size observations

All supplied `.ITM` lengths are divisible by 63 and all supplied `.SPC`
lengths by 9. These are measured size units, not yet verified item or spell
field layouts. The audit reads no sidecar content. Two character records have
no `.SPC` sidecar in this corpus; a missing spell sidecar is not treated as a
format error. Sidecars that are supplied must pair with a `.SAV` stem.

## Tests and private local audit

```sh
cmake -S . -B /tmp/poolrad-dos-character-build -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-dos-character-build
ctest --test-dir /tmp/poolrad-dos-character-build --output-on-failure
/tmp/poolrad-dos-character-build/poolrad_dos_character_audit resources/DOS/POOLRAD/CHRDAT*.SAV resources/DOS/POOLRAD/CHRDAT*.ITM resources/DOS/POOLRAD/CHRDAT*.SPC
```

Synthetic tests cover exact framing, undersized and oversized inputs,
one-past empty spans, last-byte spans, integer overflow candidates, invalid
descriptors, and unchanged outputs after errors. On 2026-10-10, the strict
build passed all six CTest targets. The audit of the ignored owner-supplied
files reported only aggregate sizes and counts:

```text
sav_files=12 sav_bytes=3420 itm_files=12 itm_bytes=1764 itm_size_units=28 spc_files=10 spc_bytes=198 spc_size_units=22 item_pairs=12 spell_pairs=10 failures=0
```

The 12 `.ITM` sizes were two at 63 bytes, four at 126, and six at 189. The
ten `.SPC` sizes were four at 9 bytes, three at 18, one at 27, one at 36, and
one at 45. The target executable SHA-256 hashes were:

```text
START.EXE  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd
GAME.OVR  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614
```

No character names, save bytes, sidecar contents, or original files are part
of the repository or audit output. Meaningful DOS field accessors require
further executable tracing or controlled save/load differences.
