# Cold-start credits synthetic RGB24 frame

This C composer renders the DOS v1.3 credits state from ignored original
assets and original unit-0x17 printer operands. Its output matches one saved
native frame exactly. This proves a same-state frame, not live startup
reachability, the first live display writer, a translated credits procedure,
or a strict full-C game-function count.

The source map and its independent read-only pixel audit are in the
[credits source contract](COLD_START_CREDITS_SOURCE_CONTRACT.md). The merged
[DAX/EGA readers](../formats/EGA.md) decode the source archive. The merged
[font and printer model](FONT_8X8D_201.md) supplies the opaque EGA glyph
behavior and uppercase-modulo-64 lookup. This implementation embeds no
original game text, tile data, font bytes, or captured frame.

## Pinned inputs and behavior

The private inputs are `8X8D1.DAX`, SHA-256
`cce9313f23a600200fec60b5d46cd08aa9535a6143eee5d5d768a1d4e0415021`,
and `GAME.OVR`, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Both remain under ignored `resources/`. The native reference is saved
`stable-frames/0443-0595.png`, PNG-file SHA-256
`5d434e35fdfcbdc62970ed61d365763497be64234ed98c491b3dcb72cd33f295`.
Its decoded 640×400 RGB24 SHA-256 is
`624f69b5f7bce90ba859eee24aee2e4ac97daf66c405b145f20534123eb28698`.

`pr_cold_credits_rgb24` loads DAX font block 201 and border block 202. It
composes the two outer boxes using group-4 items 20–22, then scans the
original unit-0x17 local body at `GAME.OVR [0x02DA,0x06F5)` for exactly
33 `RES_0709:0626` far calls. Each call must have the original 24-byte
immediate/push/string-copy setup. The composer obtains column, row, color,
and Pascal string pointer from those bytes and passes the source bytes to
the merged EGA printer model; malformed operands or out-of-bounds strings
are rejected. Uncovered cells start at EGA index zero. Finally, the standard
16-color EGA palette expands each 320×200 indexed pixel into a uniform 2×2
RGB24 block. The public CLI creates a new output file and does not run an
emulator.

## Verification

The focused synthetic test supplies its own tiny DAX archive and 33-call
OVR-shaped byte buffer. It checks border item placement, shared border row,
2× scaling, opaque overwrite by a second glyph, output-buffer preservation,
missing tile block, malformed setup, missing call, and rejected cell bounds.
No original text or art appears in the test.

The strict C11 CMake build of `poolrad_cold_start_credits_rgb` and
`poolrad_cold_start_credits_rgb_tests` passed with
`-Wall -Wextra -Wpedantic -Werror`; focused CTest passed **1/1**. A bounded
ASan/UBSan run of the synthetic test passed with
`ASAN_OPTIONS=detect_leaks=0`. LeakSanitizer could not initialize under the
environment's ptrace wrapper, so no leak-scan result is claimed.

The CMake-built CLI wrote a private 768,000-byte `render-cmake.rgb24` under
ignored `resources/oracle-runs/cold-credits-rgb24-display/`. The dedicated
comparator hash-pinned the independently saved native PNG and compared all
RGB bytes. Both decoded RGB24 streams have SHA-256
`624f69b5f7bce90ba859eee24aee2e4ac97daf66c405b145f20534123eb28698`:
**0 differing channel bytes, 0 differing pixels, no difference bounds**.
The comparator prints only hashes and counts, and requires both input paths
to contain a `resources/` directory. The project keeps these inputs ignored.

```sh
cmake -S . -B resources/build-credits-display \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-credits-display --target \
  poolrad_cold_start_credits_rgb poolrad_cold_start_credits_rgb_tests -j2
ctest --test-dir resources/build-credits-display \
  -R '^poolrad_cold_start_credits_rgb_tests$' --output-on-failure
resources/build-credits-display/poolrad_cold_start_credits_rgb \
  resources/POOLRAD/8X8D1.DAX resources/POOLRAD/GAME.OVR \
  resources/oracle-runs/cold-credits-rgb24-display/render.rgb24
python3 tools/replay/cold_start_credits_rgb24_compare.py \
  --reference resources/oracle-runs/native-cold-start-gap-004/stable-frames/0443-0595.png \
  --candidate resources/oracle-runs/cold-credits-rgb24-display/render.rgb24 \
  --candidate-format rgb24
```

The example input paths should be replaced with the private locations of
the pinned original files and independently saved frame. The actual native
PNG and original files used for this receipt were read from the native replay
worker's ignored clone; the RGB output was written only to this worker's
ignored clone.
