# Cold-start title asset to RGB24 frame

This bounded portable C renderer decodes `TITLE.DAX` block ID 2 and produces
the RGB bytes of the stable native cold-start title frame. It does not recover
the startup caller, screen timing, palette-setting instructions, or an entire
DOS routine. The original archive, native screenshot, and rendered frame stay
under ignored `resources/`.

Before implementation, `deja "Poolrad title frame native slots359 433 SHA3714eba3 portable C RGB24 asset decode render"`
and a narrower `TITLE.DAX` query were attempted. Both timed out, as did a
bounded MCP recall request, so no prior session result was reused. The
checked-in [EGA block contract](../formats/EGA.md),
[native cold-start frame ledger](NATIVE_COLD_START_SCREEN_GAPS.md), and
[standard EGA palette use in an earlier private comparison](../../tests/replay/check_slot_a_static_view.py)
provided the starting contracts.

## Bounded inputs and comparison

| Private input or result | Bound or SHA-256 |
| --- | --- |
| DOS v1.3 `TITLE.DAX` | 33,898 bytes; `7944ab87afbe0002d39479d138c78c7b6b219bd6bbc3fb6844d361f5cd4dba2b` |
| title candidate | DAX block ID 2, one 320×200 EGA item; archive block ID 1 also fits this geometry |
| native stable title | CFR slots 359–433, 640×400 RGB24; `3714eba3761b6d8520c61b61bc60b844dc63a375fa9c2bad166dee509a15e2d8` |
| C block-2 RGB24 output | 768,000 bytes; `3714eba3761b6d8520c61b61bc60b844dc63a375fa9c2bad166dee509a15e2d8` |

The independent private native reference was the ignored
`native-cold-start-gap-004/stable-frames/0359-0433.png` in the replay worker's
copy. Its decoded RGB is exactly 640×400, with nine colors and exact 2×2
pixel replication. Comparing each TITLE block's high/low-nibble index map
against the native frame, block 2 has one consistent RGB color per each of
its nine used indices and agrees at all **64,000** source pixels. Block 1 can
agree at only **29,024/64,000** source pixels even after independently
choosing each index's best RGB color. The nine used block-2 indices map to
the same-numbered entries of the standard 16-color EGA RGB palette.

The C result was compared byte for byte against the decoded native PNG:
**0/768,000 mismatched bytes and 0/256,000 mismatched pixels**. This is
same-frame asset-to-RGB parity. The standard-palette mapping is observed for
the nine colors used by this frame; the other seven entries are not exercised
by this comparison. The native video ledger is a host-window capture, not a
guest instruction stop, so this match does not identify the startup call or
prove its runtime palette path.

## Reproduce with private media

Build and run from the repository root. The output path must be a new file
under ignored `resources/`; the CLI uses exclusive creation and will not
overwrite an existing file.

```sh
cmake -S . -B resources/build-cold-title-rgb24
cmake --build resources/build-cold-title-rgb24 \
  --target poolrad_cold_start_title_rgb poolrad_cold_start_title_rgb_tests
ctest --test-dir resources/build-cold-title-rgb24 \
  -R '^poolrad_cold_start_title_rgb_tests$' --output-on-failure
mkdir -p resources/oracle-runs/cold-title-rgb24
resources/build-cold-title-rgb24/poolrad_cold_start_title_rgb \
  resources/DOS/POOLRAD/TITLE.DAX \
  resources/oracle-runs/cold-title-rgb24/title-id2.rgb24
sha256sum resources/oracle-runs/cold-title-rgb24/title-id2.rgb24
```

For a viewable private image, convert that raw output under ignored
`resources/`:

```sh
ffmpeg -nostdin -f rawvideo -pixel_format rgb24 -video_size 640x400 \
  -framerate 1 -i resources/oracle-runs/cold-title-rgb24/title-id2.rgb24 \
  -frames:v 1 -update 1 resources/oracle-runs/cold-title-rgb24/title-id2.png
```

The API accepts borrowed archive bytes, selects ID 2, validates the exact
single-item 320×200 EGA layout, and writes tightly packed, top-down 640×400
RGB24. Its output-size status reports 768,000 required bytes without a
partial write; other errors leave `written` at zero. The synthetic test checks
block selection, color conversion,
2× scaling, exact buffer capacity, malformed archive/image, and missing ID.

The comparison above used the copied original archive in this worker and the
read-only native PNG in the replay worker's ignored copy. Recompute native
RGB with `Image.open(path).convert("RGB").tobytes()` and compare against the
C output; count byte and three-byte-pixel differences separately. No native
emulator was launched for this gate.
