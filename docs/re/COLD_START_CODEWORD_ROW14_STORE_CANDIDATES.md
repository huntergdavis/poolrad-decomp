# U19 row-14 first-store candidate order

This source-only audit narrows the DOS instructions that **could** first
change a row-14 EGA plane byte during the fixed codeword print. The retained
`+02DD`, `+02E2`, `+02FD`, and `+0302` captures have RAM and host RGB, but no
synchronized EGA planes. They cannot establish whether any candidate wrote
a changed value, which page the host showed, or whether the host displayed
an earlier write late. No heavy native run was made.

Before analysis, `deja "Poolrad U19 row14 first EGA plane store caller A000
118F"` returned no indexed session match. The merged [printer route](COLD_START_CODEWORD_ROW14_PRINTER_ROUTE.md),
[font/drawer contract](FONT_8X8D_201.md), and corrected [four-plane watch
provenance](COLD_START_CODEWORD_ROW14_PLANE_WATCH.md) supplied the call ABI,
font, and host witness. Their address claims were rechecked against ignored
originals and the retained loaded RAM.

## Static call and store order

Original `GAME.OVR` U19 `+02C0..+02E2` is 34 bytes, SHA-256
`144b7073fef51cd92e155eaf08dd5c679772c7788a989432d2bafc95356647ce`.
It passes column 1, row 14, and color 10 through the direct `+02D8` printer
call. Original `RES_0709:0626..06A7` is 129 bytes, SHA-256
`b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`;
its `+0691` near call enters drawer `+01DF` once per accepted character.
This is a source route; the retained `+02DD` stop is after the printer call
and does not expose its internal store sequence.

The original EGA drawer path at image `[0x7417,0x7501)` and its captured
loaded `0F2D:+0387..+0471` counterpart are byte-for-byte identical,
234 bytes, SHA-256
`3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4`.
The correct image base is `0x7090`. The inner loop selects Sequencer map
mask `DS:25C9[3-plane]`; the retained table is four bytes, SHA-256
`1ba1475eadf8ae7a3b170515ef3c887513a6feed761b5f6f889882b809cd553f`,
giving plane masks 1, 2, 4, and 8 in loop order. Its `+03ED` color test
branches to `+043D` for a zero byte, otherwise to the glyph-byte path.
The loop iterates four planes for each of eight glyph rows.

For color 10, the static per-plane paths are:

| Plane loop | Color branch | First-page store | Second-page store |
| ---: | --- | --- | --- |
| 0 | zero | `0F2D:0445` | `0F2D:0451` |
| 1 | glyph | `0F2D:0412` | `0F2D:0438` |
| 2 | zero | `0F2D:0445` | `0F2D:0451` |
| 3 | glyph | `0F2D:0412` | `0F2D:0438` |

The two glyph-store instructions are three bytes each and hash to
`4aebe178bf3ad2484c62381353ff8bdaf04cf4204f742af578350e642510f58e`.
The two zero-store instructions are four bytes each and hash to
`5c4fa34099b77e7252dd0389cac1d60bd397e30d44577efd894a5326617ced29`.
The prior `+02DD` capture records both page-segment words as `A000`, but
that does not prove which page the host presented or whether either store
changed a plane bit.

The first fixed-source block missing from the `+02DD` host frame is
`(x=122,y=112)`, byte `A000:118F`, bit `0x20`. Its captured font bit is set,
so color 10 predicts plane bits 1 and 3 set and 0 and 2 clear at that
point. On this static path, `+0445` is the first **possible** store to the
witness byte in plane-loop order; it writes zero to plane 0, followed by
`+0451` to the second page. The first glyph-byte store for that witness is
`+0412` in plane 1. Whether any of these is the first **value-changing**
store is unknown because pre-call plane bytes were not captured. Other
earlier characters can also write row 14 before this witness cell.

## Future native gate

At one same-run stop before `38D3:02D8`, preserve all four
`A000:[0x1180,0x12C0)` plane windows, the controller map/selector state,
both page words, CPU/caller frame, RAM, and host RGB. Use instruction stops
at live `0F2D:03ED`, `:0412`, `:0438`, `:0445`, and `:0451` to record the
color branch, row/plane loop counters, page segment, and effective byte
address. For the `A000:118F` witness, compare all four plane bytes before
and after each reached store, preserving and restoring the graphics
controller state as the [watch note](COLD_START_CODEWORD_ROW14_PLANE_WATCH.md)
requires. Treat an initial `BPM` baseline event as a sample, not a write.
Repeat snapshots at immediate U19 `+02DD`, `+02E2`, `+02FD`, and `+0302`
stops to separate plane transitions from host presentation. If a safe
four-plane capture is unavailable, report that blocker; a host screenshot
alone cannot identify the first guest writer.

Reproduce the source-only audit with ignored originals and captures:

```sh
python3 tools/replay/cold_start_codeword_row14_store_candidate_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --run resources/oracle-runs/codeword-row14-p5-001
```

Only addresses, hashes, and counts appear in public output. Original code,
font, RAM, pixels, and future plane dumps remain in ignored `resources/`.
No native, C, full-frame, or identity credit follows from this note.
