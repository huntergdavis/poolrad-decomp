# U19 row-14 CPU store to EGA plane boundary

This source-only gate separates the drawer's ordered CPU byte stores from a
value-changing EGA plane store. It uses no new native run. The four retained
U19 endpoint RAM/PNG pairs do not contain synchronized plane bytes or Graphics
Controller and Sequencer state, so this gate stops before a device-byte,
page-return, or RGB timing claim.

Before analysis, `deja "Poolrad U19 row14 post CPU EGA latch mask page A000
118F plane first store"` returned no indexed session. The merged
[candidate order](COLD_START_CODEWORD_ROW14_STORE_CANDIDATES.md),
[four-plane watch](COLD_START_CODEWORD_ROW14_PLANE_WATCH.md),
[page selector](COLD_START_CODEWORD_ROW14_PAGE_SELECTOR.md), and
[paired plane-event gate](COLD_START_CODEWORD_ROW14_PLANE_EVENT_GATE.md)
supplied the source addresses and capture limitations. Their hash-locked
page-selector and plane-watch audits passed again for this note.

## Proven guest-side boundary

The original drawer image `[0x7417,0x7501)` and captured loaded
`0F2D:+0387..+0471` are byte-equal, 234 bytes, SHA-256
`3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4`.
The U19 call at live `38D3:+02D8` enters resident printer
`0F2D:+0626`; its near call at `+0691` enters drawer `+01DF` for each
accepted character. The retained `+02DD` capture is after the printer
return, not an instruction stop inside the drawer.

| CPU source path | Segment load | Store PC and next PC | Intended plane-loop mask |
| --- | --- | --- | --- |
| Glyph, first page | `+040A` reads DS:4A16 | `+0412 → +0415` | 1, 2, 4, 8 in loop order |
| Glyph, second page | `+0430` reads DS:4A18 | `+0438 → +043B` | same current loop mask |
| Zero, first page | `+043D` reads DS:4A16 | `+0445 → +0449` | same current loop mask |
| Zero, second page | `+0449` reads DS:4A18 | `+0451 → +0455` | same current loop mask |

The original glyph-store pair hashes to
`4aebe178bf3ad2484c62381353ff8bdaf04cf4204f742af578350e642510f58e`;
the zero-store pair hashes to
`5c4fa34099b77e7252dd0389cac1d60bd397e30d44577efd894a5326617ced29`.
The four-byte mask table hashes to
`1ba1475eadf8ae7a3b170515ef3c887513a6feed761b5f6f889882b809cd553f`.
These are instruction and CPU-source facts. A CPU write may preserve the
destination byte, and a write to one selected plane need not change any
other plane. The C glyph frame and ordered CPU-source model therefore
cannot themselves certify a device transition.

At the fixed source witness `(122,112)`, the byte is `A000:118F` and the
pixel bit is `0x20`. For color 10, the static source route chooses zero for
planes 0 and 2 and glyph data for planes 1 and 3. The first *possible*
witness store in loop order is zero at `+0445`; the first possible glyph
store is `+0412`. Neither is established as the first **value-changing**
store. Earlier row-14 cells can also write before this witness.

## Device and page state still needed

The retained `+02DD/+02E2/+02FD/+0302` endpoint words have
DS:4A16/4A18=`A000/A000`, page indexes 0/0, mode `0x0D`, and display-state
SHA-256 `52c20cdb7fceeb8f44f77d9d25201b13b4e056958430161515115c310de4d9d3`.
The host row mismatch counts are `477/477/477/0`; successive RGB pixel
deltas are `0/0/6612`. The bounded U19 suffix/input body has no exact
direct far-call encoding to resident active-page `+0071` or draw-page
`+00A0` helpers. These facts do not exclude an indirect or nested helper
call or a same-value page write.

At a reached CPU store, a device comparison requires the before/after four
plane bytes at the *resolved* address, Graphics Controller selector/read map,
write mode, enable set/reset, data rotate, and bit mask, Sequencer map mask
and memory mode, and the active/draw page words. A guest `BPM A000:118F`
reports one selected guest-readable byte in the pinned debugger; it does
not cover all four plane writes. The pinned ignored heavy DOSBox-X executable
has SHA-256 `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`
and contains `VGA_Generic_Write_Handler<false>` and
`VGA_Generic_Write_Handler<true>` symbols. Which handler processes this
saved U19 store is not established; the system `/usr/bin/dosbox-x` has a
different hash. This is a candidate host-side watch boundary, not a
measured device path.

The next measured gate is one paired stop at a reached candidate PC with
effective ES:DI=`A000:118F`, then its listed successor. Record all four
plane bytes and controller state before and after, restoring the selector
and read map exactly after each read. The separate page-return gate must
then bind a same-invocation helper frame to U19 and correlate plane state
with the immediate `+02DD/+02E2/+02FD/+0302` RGB endpoints. Both gates
remain held pending a serial native slot.

## Reproduction

```sh
python3 tools/replay/cold_start_codeword_row14_page_selector_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --run resources/oracle-runs/codeword-row14-p5-001
python3 tools/replay/cold_start_codeword_row14_plane_watch_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --run resources/oracle-runs/codeword-row14-p5-001
```

Only addresses, hashes, and counts are public. The originals, RAM, EGA
planes, and pixels stay in ignored `resources/`. This note adds no native,
first-writer, page-flip, RGB, full-frame, C, or identity credit.
