# U19 row-14 four-plane watch preparation

This source-only gate chooses one byte and bit for a later native EGA plane
watch of the fixed codeword row. It does **not** measure a guest plane
transition, identify the first value-changing store, or prove when the host
presents a page. No emulator run was started for this note.

Before this work, `deja "Poolrad U19 row14 EGA plane watch A000 118F drawer
0412"` and the broader `deja "Poolrad EGA plane watch"` returned no indexed
session match. The merged [row-14 printer route](COLD_START_CODEWORD_ROW14_PRINTER_ROUTE.md)
provided the four candidate store sites; the [paired-frame boundary](COLD_START_CODEWORD_ROW14_BOUNDARY.md)
provided the screenshot intervals; the [return-path audit](COLD_START_CODEWORD_ROW14_RETURN_PATH.md)
separated the `+02DD` no-op return from the `+02FD` line-input call. This
audit independently rechecks their original and loaded code windows against
the ignored binaries and captured RAM.

## Address and host-pixel provenance

Ignored `START.EXE` and `GAME.OVR` SHA-256 values are
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`
and `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The original U19 descriptor starts at `GAME.OVR` file `0x09AC`; this saved
run loads it at `38D3`. Its file `+02D8..+0302` window is 42 bytes, SHA-256
`518c6f63fa4c0ed6ff7dcf6b986fb4748ecf3e51a366f9005685d7f7e630a00c`.
The corresponding captured RAM window is 42 bytes, SHA-256
`c5b6d514596572705787fe93b84b1ae74600ce8a59ce0ecf9fd6d589071c5a06`.
The loaded far-call segments at `+02D8`, `+02DD`, `+02F2`, and `+02FD` are
each their original segment plus the measured MZ base `0x0824`; their offsets
are unchanged. Thus the preceding row-14 printer call is live `38D3:02D8`
to `0F2D:0626`. The `+02DD` call returns through the no-op `10BD:0104`
route; line input starts at `38D3:02FD` to `0F2D:09E3`.

The printer's drawer EGA path is original expanded-image
`[0x7417,0x7501)`, 234 bytes, SHA-256
`3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4`.
Its captured loaded `0F2D:+0387..+0471` window is byte-for-byte identical
and has the same hash. The earlier `+03DF..+04C9` label was offset by
`0x58`; it identified a different loaded window. The audit now asserts
original/loaded byte equality in addition to both hashes.
The four candidate store PCs are live `0F2D:0412`, `:0438`, `:0445`,
and `:0451` (original image `0x74A2`, `0x74C8`, `0x74D5`, `0x74E1`).
They are candidate glyph/zero stores on the static drawer route, not a
measured first writer.

The first fixed-source row-14 block that differs from the host screenshot
at `38D3:02DD` is source `(x=122,y=112)`, in cell 14 of the 36-character
source. Its byte address in a 320-pixel EGA scanline is
`A000:(112*40 + floor(122/8)) = A000:118F`; its bit mask is `0x20`.
The captured font and original source predict palette index 10 there. All
four doubled host pixels show index 0 at `+02DD` and index 10 at `+0302`.
The same-run screenshots retain 477 source-pixel mismatches at `+02DD`,
`+02E2`, and `+02FD`, then zero at `+0302`; host RGB changes by
`0/0/6612` pixels across those successive intervals. This proves a host
pixel witness and an interval, not a plane byte's earlier or later value.

## Bounded future capture recipe

The row-14 plane window is `A000:[0x1180,0x12C0)`, 320 bytes per plane.
At a same-run stop before the `38D3:02D8` printer call and at the immediate
`+02DD`, `+02E2`, `+02FD`, and `+0302` stops, preserve the CPU/caller frame,
display words, RAM, canonical RGB, and each of the four EGA read-map plane
windows. The earlier [printer-page capture procedure](SLOT_A_ROW9_COL38_PRINTER_PAGE.md)
selects Graphics Controller register 4 with `OUTP 3CE 04`, chooses read maps
0 through 3 with `OUTP 3CF`, and copies each `MEMDUMPBIN A000 1180 140`
result into ignored `resources/` before the next map overwrites it. Record
the controller state before sampling and restore it exactly before resuming;
if that cannot be established, stop rather than mutate the replay state.

Use `A000:118F` bit `0x20` as a **four-plane comparison point**. Palette
index 10 would have this bit set in planes 1 and 3 and clear in planes 0
and 2 under the ordinary four-plane color mapping. A `BPM A000:118F`
event alone may be an initial watch baseline or may expose only the
selected read map; it cannot establish a four-plane write. For each
subsequent event, record the guest PC, page/segment selector, effective
address, and all four plane bytes immediately before and after the event.
The four drawer PCs above are bounded instruction breakpoints to test,
alongside any earlier writer reached by the memory watch. A guest store is
the first **value-changing** writer only when its before/after plane bits
actually differ. If a breakpoint or four-plane read cannot be made without
perturbing the route, report that precise capture blocker.

Two disjoint measured follow-ons remain: (1) establish the first actual
four-plane value-changing store and its caller frame; (2) establish which
page/plane state the host presents at the `+0302` screenshot boundary.
Both are separate from p9's U17 credits-printer work and pR's resident
operand/mask note. This source note supplies no native, strict-C,
full-frame, or identity credit.

Reproduce this read-only audit with ignored inputs and existing paired
captures:

```sh
python3 tools/replay/cold_start_codeword_row14_plane_watch_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --run resources/oracle-runs/codeword-row14-p5-001
```

The command reports addresses, hashes, and counts only. Original code,
font, RAM, images, and plane captures stay under ignored `resources/`.
