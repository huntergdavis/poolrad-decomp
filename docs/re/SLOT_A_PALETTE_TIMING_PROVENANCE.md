# Saved slot-A palette snapshot timing provenance

The saved slot-A mode-run transcript now has a terminal append after the
22,030-byte prefix used by the merged [palette write-route audit](SLOT_A_PALETTE_WRITE_ROUTE.md).
The prefix still hashes to that audit's published SHA-256
`adb097bd6a16c49acacd6776c09af3383c7a1b15379742d15779acb08f700500`.
This audit parses **only that exact prefix**, so the prior evidence pin and
its four ordered hardware reports remain reproducible. The append is emulator
display/terminal teardown text, not another measured palette state.

Before investigation, bounded `deja "Poolrad saved slot A palette AC DAC timing
provenance mode transcript suffix first-view one-Up"` timed out without a
result. This audit reuses the merged
[palette write-route parser](../../tools/replay/slot_a_palette_write_route_audit.py),
[first-view RGB parser](../../tools/replay/slot_a_rgb_palette_audit.py), and
[RGB frame evidence](SLOT_A_RGB_FRAME_PALETTE.md).

## Pinned inputs and report order

The ignored mode capture is
`resources/oracle-runs/slot-a-palette-mode-p5-002/debugger-mode.typescript`.
Its current full length is 22,990 bytes and full SHA-256 is
`95d6bf83cec40bed147bb1aad176dab7cacdd9733df76fb6217420e4e5d45394`.
The 960-byte suffix has SHA-256
`ac06a70c49fbcf35f2d417487bbf0fe1a3ce0d19674ebcf75638266135eb5fe8`.
It contains no additional AC/DAC report or CS/IP query marker; the audit
rejects a different suffix or any such marker there.

| Order | Native stop or state | AC report byte offset | Relation to post-mode palette |
| --- | --- | ---: | --- |
| 0 | `0C88:00EE`, before BIOS video-mode call | 7,981 | Baseline |
| 1 | `0C88:00F3`, after the call | 11,323 | 9/16 AC entries, 49/256 DAC entries, and 68/768 DAC channels differ |
| 2 | Later stable first view | 15,528 | Exactly equal to order 1 |
| 3 | Later stable one-Up view | 18,759 | Exactly equal to order 1 |

The independent ignored first-view capture
`resources/oracle-runs/slot-a-rgb-palette-p5-001/debugger-palette.typescript`
has SHA-256
`578a577f4891dcb29d8e28eb84b12229727bba9a3989b8520baba3c0f83b5326`.
Its **final complete** AC/DAC report starts at byte 9,044 and equals the
mode-run post-call report. Its earlier startup text-mode report is incomplete
and excluded. The full address/order-only table is
[slot_a_palette_timing_provenance.csv](slot_a_palette_timing_provenance.csv).

The two runs' stable first-view screenshots have the same SHA-256
`2a776265f92f4f0a82fe73a64b0ac8d84d1beb93217630840eeec3f0ffb1b134`.
The mode run's stable and paused one-Up screenshots share a different SHA-256
`ecb89bbc83b4cafa8a50500b3a16f9cd286f0b41191d05c75c4e77db9b17e3c7`.
These are capture-identity checks; screenshots and pixels stay ignored.

## Interpretation and next gate

The observed AC/DAC change is bracketed by the resident `0464:+00EE` BIOS
mode-call boundary and its `+00F3` return. Equality at later stops establishes
the measured palette at those stops. It does not identify an individual BIOS
or port-write instruction, exclude same-value/intermediate writes, or prove
the exact palette at the first page-flush byte. The held `0AF8:16A1` direct
native stop is still required for that separate first-store timing claim.
The initial native first-store stop attempt was incomplete and remains on
hold. No retry is implied by this audit.

## Reproduction

With the two ignored captures present locally:

```sh
python3 -B tools/replay/slot_a_palette_timing_provenance.py
```

The script hash-locks the full mode transcript, its published prefix, its
terminal suffix, the independent RGB transcript, and five private screenshot
files. It checks the ordered AC/DAC reports, complete independent final
report, exact equality relations, and the checked-in address/order table.
No original game bytes, palette values, screenshots, or pixels are published.
