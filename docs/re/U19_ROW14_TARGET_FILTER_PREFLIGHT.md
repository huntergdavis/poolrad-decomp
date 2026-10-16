# U19 row-14 DEBUGBOX target-filter preflight

This is a **source-only, offline** preflight for the future first EGA-plane
writer capture. It does not start DOSBox-X, read EGA planes, identify a first
value-changing store, or establish host page/RGB timing. The
[paired plane-event gate](COLD_START_CODEWORD_ROW14_PLANE_EVENT_GATE.md) remains
the separate verifier for privately supplied before/after plane captures.

Before implementing the filter, bounded
`deja "Poolrad DEBUGBOX U19 row14 A000 118F target filter debugger preflight"`
returned no indexed sessions. The merged
[four-plane watch](COLD_START_CODEWORD_ROW14_PLANE_WATCH.md),
[candidate order](COLD_START_CODEWORD_ROW14_STORE_CANDIDATES.md), and
[page-return gate](COLD_START_CODEWORD_ROW14_PAGE_RETURN_RGB_HANDOFF.md)
supplied the caller, candidate PCs, witness, and remaining state requirements.
The original four store instructions were independently decoded from the
hash-locked ignored START image: `+0412/+0438` are three-byte
`MOV ES:[DI],DL`; `+0445/+0451` are four-byte immediate-zero
`MOV ES:[DI],0`. These instructions do not advance DI.

## Read-only debugger compatibility

The pinned ignored heavy DOSBox-X executable is 47,259,176 bytes, SHA-256
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.
It contains the DEBUGBOX, MEMDUMPBIN, Graphics Controller, Sequencer, mode,
OUTP, and BPM feature tokens required by the existing serial capture recipe.
The system `/usr/bin/dosbox-x` is 12,192,432 bytes, SHA-256
`99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`;
it fails the pinned hash and lacks four of those tokens. Token presence is
only a static compatibility screen. Neither binary was launched. The
ignored heavy source files referenced by DWARF are absent from this isolated
checkout, so this preflight does not independently revalidate debugger
implementation details claimed in the earlier plane-event note.

## Bounded frame selection

`tools/replay/cold_start_codeword_row14_target_filter.py` accepts only a
supplied same-run list of hexadecimal CS, IP, ES and DI WORDs. It arms on
`38D3:02D8`, the U19 printer call; `38D3:02DD` terminates the interval.
Within that interval it considers only these pinned drawer instruction
stops, skipping candidate hits unless effective ES:DI is `A000:118F`.

| Candidate | Required single-step successor | CPU source |
| --- | --- | --- |
| `0F2D:0412` | `0F2D:0415` | glyph byte, first page |
| `0F2D:0438` | `0F2D:043B` | glyph byte, second page |
| `0F2D:0445` | `0F2D:0449` | immediate zero, first page |
| `0F2D:0451` | `0F2D:0455` | immediate zero, second page |

The filter caps candidate hits at 128 and requires the next supplied frame
to be the direct successor of the selected store. It reports typed stops for
a missing U19 anchor, printer return before a witness, hit-cap exhaustion,
missing step, or wrong step. A paired CPU frame is still **not** a paired
EGA plane event. A future native driver must additionally retain the full
caller/stack frame, page words, Graphics Controller selector and read map,
Sequencer state, and all four `A000:[1180,12C0)` plane windows immediately
before and after the instruction, restoring debugger-selected controller
state before resuming. A `BPM A000:118F` hit is a route hint and cannot
certify a change in every plane.

The tool does not claim that a supplied JSON frame stream is authentic or
complete. The filter's `PAIRED_CPU_SOURCE_ONLY` result selects one bounded
instruction for later native review; it does not establish that this is the
first changing guest writer. The witness pixel bit is `0x20`.

Reproduce the static gate and synthetic rejection paths:

```sh
python3 tools/replay/cold_start_codeword_row14_target_filter.py \
  --debugger resources/dosbox-x-heavy-src/src/dosbox-x
python3 -m unittest tests.replay.test_cold_start_codeword_row14_target_filter
python3 tools/replay/cold_start_codeword_row14_store_candidate_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --run resources/oracle-runs/codeword-row14-p5-001
```

The original media, debugger binary, future CPU frames, EGA planes and
pixels stay under ignored `resources/`. No native, first-writer,
page/RGB, full-frame, C, or identity credit follows from this gate.
