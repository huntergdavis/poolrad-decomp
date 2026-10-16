# U19 row-14 candidate store CPU/RAM gate

This source-only follow-on adds an offline CPU/RAM check to the merged
[four-plane event gate](COLD_START_CODEWORD_ROW14_PLANE_EVENT_GATE.md). It
does not supply a new native capture, prove complete watch coverage, or name
the first value-changing guest writer. No emulator run was started.

The bounded deja query `Poolrad U19 row14 A000 118F first value changing four
plane store original bytes CPU RAM watch contract` returned no indexed match.
The merged [store candidate audit](COLD_START_CODEWORD_ROW14_STORE_CANDIDATES.md)
and [plane watch](COLD_START_CODEWORD_ROW14_PLANE_WATCH.md) supplied the four
PCs, original image map, and witness address. Their 234-byte drawer identity
and the four instruction boundaries were independently decoded again from
the ignored START image before this contract was written.

## Original instruction and loaded image contract

The original expanded START image `[0x7417,0x7501)` and retained loaded
`0F2D:+0387..+0471` are byte-identical, 234 bytes, SHA-256
`3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4`.
The START.EXE SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

| Live instruction | Original image | Length | Next PC | Source operand |
| --- | --- | ---: | --- | --- |
| `0F2D:0412` | 0x74A2 | 3 | `0F2D:0415` | `DL` glyph byte to `ES:[DI]` |
| `0F2D:0438` | 0x74C8 | 3 | `0F2D:043B` | `DL` glyph byte to `ES:[DI]` |
| `0F2D:0445` | 0x74D5 | 4 | `0F2D:0449` | immediate zero to `ES:[DI]` |
| `0F2D:0451` | 0x74E1 | 4 | `0F2D:0455` | immediate zero to `ES:[DI]` |

The two original glyph MOV spans each hash to
`4aebe178bf3ad2484c62381353ff8bdaf04cf4204f742af578350e642510f58e`;
the two zero MOV spans each hash to
`5c4fa34099b77e7252dd0389cac1d60bd397e30d44577efd894a5326617ced29`.
The ordinary x86 MOV leaves general registers, segment registers, and
status flags unchanged. At the witness `A000:118F`, the immediate successor
should therefore have the same CPU words except IP. A debugger trap flag
may differ, so the checker compares CF/PF/AF/ZF/SF/IF/DF/OF while excluding
TF. The register `DL` is the glyph instruction's source operand; the EGA
plane result can differ because of Graphics Controller mode and masks.

## Private paired capture and rejection gates

Reuse the merged plane-event manifest and add a `cpu` object to **both**
samples of every candidate instruction pair. Each object supplies 16-bit
integer `ax`, `bx`, `cx`, `dx`, `si`, `di`, `bp`, `sp`, `cs`, `ds`, `es`, `ss`,
`ip`, and `flags`. Keep the before/after full 640-KiB conventional RAM
files, four 320-byte EGA plane windows, and restored GC selector/read map
from the merged gate. The CPU should point to the pinned PC/successor, with
`ES:DI=A000:118F` at both stops. The single MOV must not alter captured
conventional RAM; a difference means the pair is not a clean isolated step
and requires investigation, not attribution to the EGA store. Preserve the
original full CPU and caller stack records privately for review.

The new checker first runs the merged four-plane event validator, then
hash-locks original instruction spans and the loaded drawer image, checks
each paired CPU step, and compares all conventional RAM bytes. Its public
output contains only candidate PC counts and a count of supplied witness
changes. It does not assert the debugger caught every writer: an unlisted
PC, a changed witness byte between supplied pairs, or an unsafe plane read
is a typed capture blocker. A first **actual** value-changing store requires
an independently complete native watch across the interval before
`38D3:02D8` through the immediate `+02DD` return.

Synthetic verification without game media:

    python3 -m unittest tests.replay.test_cold_start_codeword_row14_cpu_store_gate

Future private capture analysis, only after a serial native slot is cleared:

    python3 tools/replay/cold_start_codeword_row14_cpu_store_gate.py \
      --exe resources/DOS/POOLRAD/START.EXE \
      --run resources/oracle-runs/PRIVATE-RUN \
      --manifest resources/oracle-runs/PRIVATE-RUN/plane-events.json

Original bytes, captured RAM, EGA planes, and pixels stay under ignored
`resources/`. This source gate supplies no live first-writer, page/host,
full-frame, C, or identity credit.
