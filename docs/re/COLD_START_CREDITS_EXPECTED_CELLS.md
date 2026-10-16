# Cold-start credits expected-cell map

This C adapter builds a deterministic cell map from the original DOS v1.3
unit-0x17 credits source. The [33-call census](NATIVE_COLD_START_CREDITS_33_CALL_CENSUS_GATE.md)
supplies the interval and source-bank bounds; the [source contract](COLD_START_CREDITS_SOURCE_CONTRACT.md)
supplies the 40×25 coordinate convention. A bounded pre-implementation
`deja` attempt encountered a damaged index and yielded no usable result.
No native emulator was started for this change.

The ignored `GAME.OVR` input is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The original U17 caller interval `+0258..+0615` maps to file
`[0x0032C,0x006E9)`, 957 bytes (`33 × 29`), SHA-256
`11033337def20b92005c394f474a839512f651350e12df9fcfbec1fb450c77d6`.
The 29 contiguous Pascal source records at U17 `+004B..+0206`, file
`[0x0011F,0x002DA)`, total 443 bytes, hash
`b4905f882b908d6a8a22b883b953df6959ca84cf7a887f07904927c915f8b66c`.
The retained loaded caller interval hashes
`f1e1c9d1b43dbe78f79a8f388084d92baa35e4e5d21a07b26317da9dbb46a518`;
its 132 differing bytes are confined to far-call segment operands.

`pr_cold_credits_expected_cells` checks every 29-byte caller layout,
source-record start, local destination size, row, column, color, and overlap.
It requires 29 distinct sources, four used twice, and exactly 459 placed
characters. It returns one caller-ordered record per cell with numeric row,
column, color, call/text indexes, and a source byte read from the supplied
image. No game text is compiled into the library. Malformed input and a short
output buffer leave caller output untouched.

The strict C11 focused synthetic test passed **1/1** with
`-Wall -Wextra -Wpedantic -Werror`. A private read-only smoke on the ignored
hash-pinned image returned 459 cells: color indices 10, 11, and 14 occur
80, 255, and 124 times. Only counts and coordinates were printed; the
private text-bearing output remains under ignored `resources/`.

This is a static expected map. It does not show a native per-cell writer,
an EGA page, same-run host RGB, rendered glyph pixels, a translated U17
procedure, full-frame parity, or a game-function count.
