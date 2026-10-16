# Synthetic title-to-credits transition fixture

This fixture joins the two merged synthetic cold-start frame renderers into
a deterministic two-frame RGB24 sequence: title at frame index 0, credits at
frame index 1, and a boundary at index 1. The adjacency and boundary are
fixture choices. They do not assert DOS timing, native instruction reach,
intermediate frames, the first live writer, or an additional game-function
translation.

Before implementation, bounded `deja "Poolrad title credits transition
synthetic fixture first box writer +16B7 saved plane capture"` failed because
its local `index.db.lock` was read-only. The fixture instead reuses the
checked-in [title asset and RGB evidence](COLD_START_TITLE_ASSET_PROVENANCE.md)
and [credits source and RGB evidence](COLD_START_CREDITS_SYNTHETIC_RGB.md).
No new native emulator was started.

## Composition contract

`pr_cold_title_credits_fixture` takes caller-owned original `TITLE.DAX`,
`8X8D1.DAX`, and `GAME.OVR` bytes. It invokes the merged title renderer for
frame 0 and the merged credits renderer for frame 1. Each frame is 640×400
top-down RGB24, 768,000 bytes. The output is their ordered concatenation,
1,536,000 bytes. The API reports boundary frame index 1 only on success.
It stages both frames before copying either into caller output, so a title
or credits failure leaves the output unchanged. The CLI creates a new
`.rgb24seq` file at the supplied path; the evidence run placed it under
ignored `resources/`.

The synthetic test supplies small constructed DAX archives and an OVR-shaped
33-call buffer. It checks title-before-credits order, boundary index, 2×
RGB sampling on both sides, output capacity, and preservation of output when
the second renderer fails after the first has succeeded. It also checks a
title failure. No original text, art, glyph bytes, or capture is tracked.

## Private same-state receipt

The read-only original inputs were `TITLE.DAX`, SHA-256
`7944ab87afbe0002d39479d138c78c7b6b219bd6bbc3fb6844d361f5cd4dba2b`,
`8X8D1.DAX`, SHA-256
`cce9313f23a600200fec60b5d46cd08aa9535a6143eee5d5d768a1d4e0415021`,
and `GAME.OVR`, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The CMake-built CLI wrote a private sequence only under ignored
`resources/oracle-runs/title-credits-fixture-display/`. Its 1,536,000 bytes
have SHA-256
`b6b746778e2dfff3ed7e662189c437bb3fba9ae7b846fc7cea943d610ed3aaff`.

| Fixture frame | Saved native stable slots | Candidate/native decoded RGB24 SHA-256 | Differing channel bytes | Differing pixels |
| --- | --- | --- | ---: | ---: |
| 0, title | 359–433 | `3714eba3761b6d8520c61b61bc60b844dc63a375fa9c2bad166dee509a15e2d8` | 0 | 0 |
| 1, credits | 443–595 | `624f69b5f7bce90ba859eee24aee2e4ac97daf66c405b145f20534123eb28698` | 0 | 0 |

The native capture has other slots between these stable groups. This
two-frame fixture omits them by contract and does not model their content
or duration.

The strict C11 CMake build of the dedicated tool and test passed under
`-Wall -Wextra -Wpedantic -Werror`; focused CTest passed **1/1**. A bounded
ASan/UBSan test passed with `ASAN_OPTIONS=detect_leaks=0`, because
LeakSanitizer cannot initialize under this environment's ptrace wrapper.

```sh
cmake -S . -B resources/build-title-credits-fixture \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-title-credits-fixture --target \
  poolrad_cold_start_title_credits_fixture \
  poolrad_cold_start_title_credits_fixture_tests -j2
ctest --test-dir resources/build-title-credits-fixture \
  -R '^poolrad_cold_start_title_credits_fixture_tests$' --output-on-failure
resources/build-title-credits-fixture/poolrad_cold_start_title_credits_fixture \
  resources/POOLRAD/TITLE.DAX resources/POOLRAD/8X8D1.DAX \
  resources/POOLRAD/GAME.OVR \
  resources/oracle-runs/title-credits-fixture-display/title-credits.rgb24seq
```

The example asset paths should be replaced with the private locations of
the pinned original files. Both native reference PNGs and original assets
used for the receipt were read from the native replay worker's ignored
clone; the derived sequence stayed in this worker's ignored clone.
