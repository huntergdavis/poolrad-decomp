# Cold-start publisher splash asset to RGB24 frame

The portable C publisher renderer decodes `TITLE.DAX` block ID 1. It produces
the stable native publisher splash's RGB24 bytes, independent of the title
renderer, credits renderer, and startup connector. Original and derived media
remain under ignored `resources/`.

Before analysis, bounded `deja "Poolrad publisher splash TITLE.DAX block1 native slots 248 352 RGB 15e405 asset palette"`
timed out. No prior session result was reused. The checked-in
[DAX/EGA contract](../formats/EGA.md),
[native cold-start screen ledger](NATIVE_COLD_START_SCREEN_GAPS.md), and
[standard EGA palette reference](../../tests/replay/check_slot_a_static_view.py)
supplied the starting layout and comparison contracts.

## Original asset and saved native frame

| Private input or result | Bound or SHA-256 |
| --- | --- |
| DOS v1.3 `TITLE.DAX` | 33,898 bytes; `7944ab87afbe0002d39479d138c78c7b6b219bd6bbc3fb6844d361f5cd4dba2b` |
| publisher candidate | DAX block ID 1, one 320×200 EGA item |
| saved native publisher splash | CFR slots 248–352, 640×400 RGB24; `15e405dac036c9530d5f87ccb67c9427012e6860195ada76b56193ed03c557dc` |
| C publisher RGB24 output | 768,000 bytes; `15e405dac036c9530d5f87ccb67c9427012e6860195ada76b56193ed03c557dc` |

The read-only native reference was the ignored
`native-cold-start-gap-004/stable-frames/0248-0352.png` in the replay
worker's copy. It contains ten colors and exact 2×2 replication of a
320×200 source image. Against that native frame, block 1 has
**64,000/64,000** source pixels consistent with one RGB color per index and
zero conflicting indices. Its ten used indices map to the same-numbered
standard EGA RGB entries. Block 2 reaches only **29,555/64,000** pixels even
when each index is assigned its individually best native color.

This establishes that block 1 alone supplies every pixel of the observed
stable publisher frame. Any text visible within that image is part of this
raster for this frame; no separate text composition is needed to reproduce
it. This does not prove the DOS draw call, guest timing, or hardware palette
instructions. The native record is a host-window frame, not an instruction
stop. The other six palette entries are not exercised by this frame.

The C output and native RGB each have 768,000 bytes and identical SHA-256:
**0 mismatched bytes and 0 mismatched pixels**. This is asset-to-frame parity,
not startup route or full-routine credit. No new native run was used.

## Reproduce with ignored original media

From the repository root, build and run. The CLI exclusively creates a new
output file and will not overwrite an existing path.

```sh
cmake -S . -B resources/build-cold-publisher-rgb24
cmake --build resources/build-cold-publisher-rgb24 \
  --target poolrad_cold_start_publisher_rgb poolrad_cold_start_publisher_rgb_tests
ctest --test-dir resources/build-cold-publisher-rgb24 \
  -R '^poolrad_cold_start_publisher_rgb_tests$' --output-on-failure
mkdir -p resources/oracle-runs/cold-publisher-rgb24
resources/build-cold-publisher-rgb24/poolrad_cold_start_publisher_rgb \
  resources/DOS/POOLRAD/TITLE.DAX \
  resources/oracle-runs/cold-publisher-rgb24/publisher-id1.rgb24
sha256sum resources/oracle-runs/cold-publisher-rgb24/publisher-id1.rgb24
```

For a viewable private PNG:

```sh
ffmpeg -nostdin -f rawvideo -pixel_format rgb24 -video_size 640x400 \
  -framerate 1 -i resources/oracle-runs/cold-publisher-rgb24/publisher-id1.rgb24 \
  -frames:v 1 -update 1 \
  resources/oracle-runs/cold-publisher-rgb24/publisher-id1.png
```

The API accepts borrowed archive bytes, selects block ID 1, validates its
single 320×200 EGA item, and writes top-down 640×400 RGB24 with exact 2×
scaling. On `OUTPUT_SIZE`, it reports 768,000 bytes without partial output;
other errors report zero bytes written. Synthetic tests check block selection,
palette mapping, 2× scaling, exact capacity, and invalid inputs. The saved
native PNG comparison decodes it with
`Image.open(path).convert("RGB").tobytes()` and counts byte and pixel
differences separately. Both private inputs must stay under `resources/`.
