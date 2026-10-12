# DOS v1.3 world-save frame

`SAVGAM*.DAT` is a fixed 13,137-byte world-state file in the audited DOS
v1.3 build. `pr_dos_savgam_open` borrows one complete file; it does not load
characters, run scripts, interpret unknown fields, or write a save. The
original-game save slot also needs its matching `CHRDAT*` character records
and optional sidecars. No original save bytes are included here.

This investigation used `deja "SAVGAM world file DOS Pool of Radiance"`;
the result was the Claude worker's current isolated save audit. The transfer
table and the two state lookups below were independently rechecked against
`GAME.OVR` SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The executable's unit `0x6f` begins at file offset `0x1dfe6`. Its public
entry 8 (`+0x1602`) reads the frame and entry 9 (`+0x1d58`) writes it. The
reader's ten transfers at `+0x17d5..+0x18da` and writer's matching transfers
at `+0x1f1c..+0x20b4` use the lengths and destinations below. The calls to
the Turbo Pascal runtime I/O routines use the same file handle throughout.

| File offset | Bytes | DOS load destination | Evidence |
| ---: | ---: | --- | --- |
| `0x0000` | 1 | `DS:0x5376`, initial ECL file byte | reader `+0x17db..+0x17ea` |
| `0x0001` | `0x800` | far pointer `[0x49D2]`, window 0 | `+0x17f5..+0x1805` |
| `0x0801` | `0x800` | far pointer `[0x49D6]`, window 1 | `+0x1810..+0x1820` |
| `0x1001` | `0x400` | far pointer `[0x49DA]`, window 2 | `+0x182b..+0x183b` |
| `0x1401` | `0x1e00` | far pointer `[0x49DE]`, ECL buffer | `+0x1846..+0x1856` |
| `0x3201` | 5 | `DS:0x6AAD..0x6AB1`, party position and two unclassified bytes | `+0x1861..+0x1870` |
| `0x3206` | 1 | `DS:0x49FA`, post-load selector | `+0x187b..+0x188a` |
| `0x3207` | 1 | `DS:0x49F3`, mode byte | `+0x1895..+0x18a4` |
| `0x3208` | 1 | local character count | `+0x18af..+0x18bf` |
| `0x3209` | `0x148` | eight 41-byte basename slots | `+0x18ca..+0x18da`; writer stride at `+0x2052..+0x2069` |

The ten widths sum to `0x3351` bytes. Both owner-held A and J world files
have that exact length. A private, read-only comparison found 289 differing
byte positions: 5 in window 0, 3 in window 1, 45 in window 2, none in the
ECL buffer, 3 in the five-byte party block, and 233 in the basename table.
These counts are evidence about the two supplied files, not a general
validity test for saves.

## Fields needed for the first native replay

| Field | File offset | DOS use | Confidence |
| --- | ---: | --- | --- |
| `x`, `y`, facing | `0x3201..0x3203` | loaded into `DS:0x6AAD..0x6AAF` | byte-verified transfer; A/J observations agree with displayed position and facing |
| map ID, low byte | `0x01e5` | window 0 offset `0x1e4`; gameplay loop unit `0x1c:+0x3321..+0x332a` copies its low byte to `[0x84DC]` | byte-verified read |
| saved ECL file byte | `0x0000` | initially loaded to `[0x5376]` | byte-verified transfer |
| restored ECL file byte | `0x0e25` | window 1 offset `0x624` overwrites `[0x5376]` at unit `0x6f:+0x1bc1..+0x1bca` | byte-verified post-load fixup |
| mode byte | `0x3207` | loaded to `[0x49F3]`; the saved-game menu returns when nonzero and sets `[0x49F8] = 1` | byte-verified gate; meanings of individual values remain partial |
| character count | `0x3208` | loop bound for subsequent `CHRDAT*` loads | byte-verified use |

The A fixture's observed `15,4 N` corresponds to its party bytes `15,4,6`;
the J fixture's observed `14,5 S` corresponds to `14,5,4`. Thus codes 6 and
4 display as N and S in these runs; other direction codes are unclassified.
The window 0 position words at offsets `0x1e0..0x1e3` do not match these
party bytes in either file, so the five-byte party transfer is the direct
load-time position source. The script buffer at `0x1401..0x3200` must also be
available when the restored route runs its ECL search entry.

The read-only C API returns spans for all ten blocks and these raw core
fields. It checks the exact frame size, but a correctly sized arbitrary byte
array can still be an invalid game save. Its synthetic test has distinct
nearby bytes to catch off-by-one field reads; no original data is embedded.

## Remaining connected contracts

This frame does not make a native game route. The biggest missing behavior
for the already observed one-step DOS replay is the movement/input path:
gameplay loop unit `0x1c:+0x32f8` and unit `0x68` entries 1 and 2, followed
by ECL run/search effects. The unit `0x6f` loader also conditionally calls
the wallset entry and feeds window word pairs to the pieces entry; those
post-load calls are decoded but not reproduced by this reader. Save/reload
will require the mirrored world writer plus character and sidecar writing.
The first native route is an integration milestone toward full DOS behavior,
not a completion percentage or a substitute for the executable routine
inventory.
