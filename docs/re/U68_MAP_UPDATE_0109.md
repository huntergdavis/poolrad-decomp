# Unit 0x68 internal +0x0109: map-field update

`pr_u68_map_update` translates the DOS v1.3 `GAME.OVR` routine at unit
`0x68:+0x0109..+0x0235`. For in-grid x/y and selector 6, 4, 2, or 0,
it updates one two-bit field in the far map byte at
`DS:[0x6A5C] + 0x300 + 16*y + x`. All decoded target paths and its
single memory effect are represented for a supplied full ES segment
view. PM postmerge parity review accepted a complete game-specific C translation
and verified identity. The **live** far pointer selector and backing contents
have not been captured, so saved one-Up reachability remains unproved.

Before decoding, `deja "Poolrad unit 0x68 +0x0109 +0x0235 map field updater 0x6A5C"`
found no exact prior session. Reused and independently byte-checked
the two-call context from [the merged +0x04DF note](U68_MOVEMENT_FLAG_04DF.md)
(PR #155) and the unit row in the [overlay function ledger](overlay_function_ledger.csv).
The CMake target is uniquely named; p6 was notified before the shared
file edit.

## Hash-locked span, arguments, and map layout

The ignored `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x68` starts at file `0x166CB`. This routine spans file
`0x167D4..0x16900` inclusive: **301 bytes / 125 contiguous instructions**,
SHA-256 `3f9a24eb07cda0c97f99a02fbd295fd7c4f55914e035d296b7e2b6ea7462d554`.
The `retf 6` ends at `+0x0236`, exactly the next prologue. The body
has no calls or indirect jumps.

The three Pascal word arguments are at `[bp+0xA]` x, `[bp+8]` y, and
`[bp+6]` selector; this routine reads **only their low bytes**. The
caller `+0x04DF` pushes x/y/facing and calls here at `+0x055D`, then
computes a stepped pair and calls again at `+0x059B` with selector
`(facing+4) mod 8`. Each call uses `push cs; call near +0x0109`, and
`retf 6` discards the three words. The live execution of either call
during the saved one-Up replay remains unproved.

`LES DI, [0x6A5C]` loads the far map base. Every selected branch adds
`16*y`, then `x`, to 16-bit DI, and reads/writes one ES byte at
`DI+0x300`. The effective offset wraps modulo 65536. With a base offset
of zero, x/y 0..15 address the 256 bytes at relative offsets
`0x300..0x3FF`. This describes the instruction layout; it does not
establish the live allocation size, backing selector, or original
map contents.

## Complete branches and effects

| Path | Exact effect on reached byte `v` |
| --- | --- |
| x or y outside signed byte range 0..15 (`+0x010C..+0x0124`) | Return without loading the far pointer or writing memory. |
| selector 6 (`+0x0127..+0x016A`) | `v := (v & 0x3F) | 0x40` |
| selector 4 (`+0x016D..+0x01AD`) | `v := (v & 0xCF) | 0x10` |
| selector 2 (`+0x01B0..+0x01F0`) | `v := (v & 0xF3) | 0x04` |
| selector 0 (`+0x01F2..+0x0232`) | `v := (v & 0xFC) | 0x01` |
| any other selector | Return without loading the far pointer or writing memory. |

There are no DS writes and no memory writes outside that one ES byte.
The callee leaves hardware registers with path-specific values, but
neither `+0x04DF` call consumes them; it is a Pascal procedure here.
The C API receives the map base offset and the matching mutable 64 KiB
ES view explicitly. Missing or short views return a host-only ARGUMENT
status **only when a map byte is reached**, before any write. The DOS
routine has no corresponding host guard.

## Synthetic verification

`test_overlay_u68_map_update.c` uses a fabricated full segment. It
checks all four selector masks across all 256 starting byte values,
unchanged neighboring bytes, x/y row-stride order, the admitted 15,15
edge with 16-bit offset wrap, all out-of-grid coordinate bytes,
unsupported selectors, and a reached missing/short view. No original
map bytes, decoded media, or private replay data are committed.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_0068_0109 --require-current-main
```
