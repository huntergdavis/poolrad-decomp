# Saved slot-A: address map of 615 pre-flush object differences

The checked-in [offset map](slot_a_object_615_offsets.csv) lists every
offscreen-object address that differs between the completed view-four
placement and the later entry to resident `0464:1B52`. It contains
addresses and capture-window labels only: no object values, art, or pixels.
The read-only [audit](../../tools/replay/slot_a_object_615_offset_audit.py)
regenerates the CSV from eight hash-locked, ignored 640-KiB RAM captures.
The first page-store C candidate remains on a separate local HOLD pending
a direct native `+16A1` stop; this map supplies no page-store parity.

Before this static audit, `deja "Poolrad saved slot A first-view entry
flush 615 earlier offscreen object bytes differing static per-offset writer
provenance"` was timeboxed without a result. Reused the merged
[last-six tile composition](SLOT_A_U3BD_VIEW4_LAST6_COMPOSITION.md),
[first later value-changing tile](SLOT_A_POST_VIEW4_FIRST_OFFSCREEN_WRITER.md),
and [first dirty staging row](SLOT_A_RES464_FIRST_STAGE_ROW.md).

## Capture provenance

The two runs share complete 14,135-byte objects at their respective
placement-return and flush-entry stops. Their whole-RAM files are different
and are not treated as interchangeable. All captures reside under ignored
`resources/oracle-runs/`; the audit pins the following full-RAM hashes and
checks the captured `DS:[4A20]` far pointer resolves to physical `0x3DE48`.

| Run directory | Capture | RAM SHA-256 |
| --- | --- | --- |
| `slot-a-view4-last6-compose-p5-001` | `ram-placement-return.bin` | `e05bb3cc8974b4bef870c7fbaf46177b09b9531f16b92eb19f4c7f6c0b4e2483` |
| same | `ram-flush-entry.bin` | `48becc9f9848794f232e6887a08edfd8b59d901833ccffbbbea4c336245080d7` |
| `slot-a-view4-writer-order-p5-002` | `ram-current-return.bin` | `871e7f3d42cb590c4328419977749baa2c4e9dfb6059af13dc21b265b1cb7ada` |
| same | `ram-after16-first-tile-pre.bin` | `f70133941a9737e887618286699ee94e8885505679392b486d0a7823dd75fc0f` |
| same | `ram-after16-first-tile-post.bin` | `82faf7062da4a2c3e1abdd7846d3e64cad2e7aab9533c61ff2d423396f0a19b4` |
| same | `ram-after16-second-tile-pre.bin` | `391a759cf5e9083156af3e4114ff0b1913a446cdc7b89cd569c3c12846162e8d` |
| same | `ram-after16-second-tile-post.bin` | `224fad0a97131d157756426bc2b1912ff961498dbb391bdee194bf13cdb3dc3b` |
| same | `ram-flush-entry.bin` | `d0ce92b6a3da99664fef97111ff9c82129307c377fc789ab5e1cbaf7de37df17` |

The baseline object SHA-256 is
`8664637ddd0016f3ef9978c4694f87ecbb7fb325ab147475f936c410ce9c6e8d`;
the flush-entry object SHA-256 is
`489572698be6487b4d9c88b470f62b530e0bd98539c99b75444a76b3639b14e1`.
The audit pins the intermediate object hashes as well. The CSV itself
has SHA-256
`aef490552378d1f9af3aa4ded45097ec0e55af1407d61780a9b252bddec84e9d`.

## Address interpretation

Object bytes `0..22` are the header. The remaining 168 rows use 84
interleaved bytes per row, with four plane bytes per byte-column. For
each CSV offset `o`, `row=(o-23)//84`, `byte_column=((o-23)%84)//4`,
and `plane=(o-23)%4`. `physical_address=0x3DE48+o` refers only to
these captured runs.

All **615** final-differing offsets are data bytes in rows **17..103**:
174 in plane 0, 219 in plane 2, 222 in plane 3, and zero in plane 1.
The first later tile leaves the object unchanged. The second later
tile changes 16 of these positions. Eight retain the second-tile value
at flush entry; eight have another value there. The remaining **599**
positions first differ at the next captured stop, flush entry. The CSV
calls these three observation classes `second_tile_retained`,
`second_tile_overwritten`, and `later_unattributed`.

The [first-writer trace](SLOT_A_POST_VIEW4_FIRST_OFFSCREEN_WRITER.md)
independently attributes the second-tile change to the reached item-40
store at resident `0464:+0C6A`. This map does **not** identify the
later instruction(s) responsible for the overwritten eight or the 599
other offsets. A capture window cannot distinguish one writer from
several, same-value writes, or temporary changes. No continuous
placement-to-flush C composition, visible page, RGB, or full-frame claim
follows from this static comparison.

Run `python3 tools/replay/slot_a_object_615_offset_audit.py` to check
the pinned captures against the checked-in CSV. `--emit-csv` prints
only the address map for independent comparison.

## Disjoint next static map

Queue a palette/RGB timing-provenance map using the **existing** ignored
`slot-a-palette-mode-p5-002` and `slot-a-rgb-palette-p5-001`
captures. The merged [palette transition](SLOT_A_PALETTE_WRITE_ROUTE.md)
brackets the AC/DAC change across the resident `0464:+00EE` BIOS
video-mode call; the merged [RGB frame](SLOT_A_RGB_FRAME_PALETTE.md)
pins the stable first-view state. The next map should list capture
stop, hardware-query provenance, and equality relationships, while
keeping individual BIOS port-write attribution unknown. It requires
no emulator run and is separate from the 615 object offsets.
