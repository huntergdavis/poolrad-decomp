# Saved slot-A post-wall flush: all reached video-state-1 dirty rows

The saved post-wall call to `RES_0464:1B52` reaches video state 1
with **88 dirty rows**, numbered 16 through 103. A bounded portable C
model now processes every one of them: it stages interleaved offscreen
object bytes into all four `A200` planes, then models the latch-backed
runtime Move into all four `A000` planes. Pre-flush object/RAM,
Graphics Controller state, staging planes and page planes are inputs.
No original output bytes, screenshot decoder or screen pixels are used
as a predictor.

The C output matches the immediate same-run native capture at
**7,744/7,744 dirty plane-byte positions** across staging and page.
Across the entire captured union, including unchanged gaps, it matches
**27,928/27,928 plane bytes**. Predicted dirty metadata matches
**168/168 rows**. This is a bounded reached-path gate, not a full
translation of the 778-byte procedure across its other video modes.
It adds no strict full-C or game-identity count.

Before debugging, `deja "Poolrad saved slot A RES_0464 1B52 all dirty
rows A200 A000 video state 1 latch Move extents"` and
`deja "RES_0464:1B52"` found no indexed sessions. Reused and
independently checked the [post-wall address path](SLOT_A_U40C_DISPLAY_TRANSFER.md)
(PR #319), [first EGA byte](SLOT_A_FIRST_EGA_INDEXED_BYTE.md)
(PR #322), and the [accepted row-56 latch model](SLOT_A_U40C_ROW56_TRANSFER_MODEL.md)
(local base `abd3be0`). This new branch stays local under PM's
no-GitHub-write instruction.

## Every reached extent

The hash-pinned flush-entry RAM has `DS=14A0`, video-state byte
`DS:[4A1E]=1`, selected page `A000`, and far offscreen object
at physical `0x3DE48`. The exact dirty-row set is the contiguous
integer range `r=16..103`. The audit reads and checks each row's
`DS:[4A28+r]`, first/last words, low offset, stage row base and
page base. Their measured values are fully enumerated by:

```text
first(r)       = 1352 + 84*(r-16)
last(r)        = 1395 + 84*(r-16)
low(r)         = 2
A200_start(r)  = 0x03C3 + 40*(r-16)
A000_start(r)  = 0x03C3 + 40*(r-16)
length(r)      = ((last-first) >> 2) + 1 = 11 bytes
```

Thus the 88 nonoverlapping spans cover 968 byte offsets per plane
within union `[0x03C3,0x1166)`. The 2,523 intervening offsets per
plane are captured as pre-flush inputs and remain unchanged by this
flush. The first/last series and stage/page offsets are checked for
every adjacent row by the audit, not inferred from a single row.

## Original instruction and EGA boundary

Ignored DOS v1.3 `START.EXE` is SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The original flush file span `[0x61CB,0x64D5)`,
`RES_0464:1B52..1E5B`, is 778 bytes / 305 aligned instructions,
SHA-256
`7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c`.
Runtime Move `0AF8:1685..16A8` is file `[0xC2FB,0xC31E)`,
35 bytes / 16 aligned instructions, SHA-256
`1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972`.

The native stop is `0C88:1B52`, with return `2D14:05FB` and
outer return `2D14:0DAE`; the second stop is at that immediate
`2D14:05FB` return. The heavy debugger's live Graphics Controller
reads at entry give registers 0/1/3/4/5/8 =
`00/00/00/00/00/FF`. Its ignored command/result record is SHA-256
`4a474c34200525498a5a223630e9a858db3902bb713d4191c70b7214d5bd6ce5`.
The probe takes those observed values as inputs and rejects other
write configurations.

| Original offset | Reached effect |
| --- | --- |
| `+1BD1..+1D2E` | Select Sequencer map-mask register 2, scan all 168 dirty flags; each dirty row's four plane loops use `DS:[25F6..25F9]` values 1, 2, 4, 8 and store interleaved object bytes at `+1C37,+1C7E,+1CC6,+1D0F` into `A200`. |
| `+1D31..+1D49` | Set map mask `0F` and Graphics Controller register-5 write mode 1. |
| `+1D51..+1DAE` | Scan dirty rows again. At `+1DA0`, call runtime Move from each `A200` span to the corresponding `A000` span. |
| `+1DB0..+1DB5` | Restore write mode 0. |
| `+1E25..+1E53` | FillChar resets all 168 dirty flags to 0, first extents to `FFFF`, last extents to 0. |

The staging pass uses measured pre-flush write mode 0, bit mask
`FF`, no data rotate or enabled set/reset; each CPU byte replaces
the planes enabled by its current map mask. The Move body loads far
source/destination/length at `+1689/+168D/+1691`, executes `CLD`,
and reaches forward `REP MOVSB` at `+16A1` because source and
destination offsets match. In EGA write mode 1, each `A200` read
loads all four latches and each `A000` write transfers those four
latched bytes under map mask `0F`. The [DOSBox-X VGA memory implementation](https://github.com/joncampbell123/dosbox-x/blob/master/src/hardware/vga_memory.cpp)
independently defines the same read-latch, write-mode-1 and map-mask
operations. The C model keeps the staging pass and Move pass separate,
as the original does.

## Same-run private comparison

The ignored run
`resources/oracle-runs/slot-a-u40c-all-dirty-p5-001/` contains
640 KiB RAM snapshots with SHA-256
`25ae4cdbceb249e04ec413b49e6cbfc6f14ff4210378bbdd34e19d37389266d1`
at entry and
`c8efd7433d584cf25303861be3b2e6aec9bcf8b7144631196e3e24ff0712e959`
at immediate return. For both `A200` and `A000`, all four EGA
planes were sampled over `[0x03C3,0x1166)` before and after the
flush; the read-map was restored before resuming. The audit pins all
16 exact 3,491-byte capture hashes and the GC record hash.

| Comparison | Result |
| --- | ---: |
| Pre-flush offscreen object to native post-flush staging, dirty spans | 3,872/3,872 |
| Native post-flush staging to page, dirty spans | 3,872/3,872 |
| C model to native post-flush staging and page, dirty spans | 7,744/7,744 |
| C model to native post-flush staging and page, full union including gaps | 27,928/27,928 |
| C metadata to native post-flush row flags/first/last | 168/168 |

The native flush changes 5,570 plane bytes across both segments
within dirty spans and changes zero bytes outside them in the captured
union. All 88 reached rows satisfy the model's aligned-extent and
mapped-source bounds; the audit reports zero unsupported reached
rows. Synthetic tests cover two distinct dirty rows, the staging
pass before any Move, overlapping map masks, unselected staging
planes, an empty dirty set, reset of all metadata, and host stops
that preserve inputs. Original DOS code has no host allocation
bounds checks; other video-state branches, invalid far pointers,
and full-procedure parity remain outside this model.

```sh
cmake -S . -B resources/build-u40c-all-dirty \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u40c-all-dirty \
  --target poolrad_resident_464_dirty_transfer_tests \
           poolrad_slot_a_ega_all_dirty_probe
ctest --test-dir resources/build-u40c-all-dirty \
  -R '^poolrad_resident_464_dirty_transfer_tests$' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/replay/slot_a_ega_all_dirty_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --run resources/oracle-runs/slot-a-u40c-all-dirty-p5-001 \
  --probe resources/build-u40c-all-dirty/poolrad_slot_a_ega_all_dirty_probe
```

Original game files, RAM, plane captures, and private images remain
under ignored `resources/`; no image or pixel array is published.
