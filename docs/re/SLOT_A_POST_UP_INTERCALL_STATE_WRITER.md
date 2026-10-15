# Slot-A post-Up intercall party and window writer

This note identifies the reached state writer between the first post-Up
printer return and the row-15 clear entry. In one controlled slot-A replay,
unit 0x68's coordinate writer changes party X, then its bit-set call invokes
unit 0x7d entry 1 with `(n, idx) = (1, 2)`. Unit 0x7d writes window-0 word
`+0x190` from 5 to 0 and carries into `+0x192` from 10 to 11. The same
party/window values persist at the clear entry.

The required `deja "Poolrad post Up second printer party 15 4 6 to 14 4 6
window 0 5 10 to 0 0 11 state writer"` search found no reusable indexed
session. The merged [U68 coordinate-writer map](U68_COORDINATE_WRITER_073B.md)
supplied the candidate bit-set call at `+07E0`; the merged
[U7D entry-1 map](U7D_ENTRY1.md) supplied its seven-word write-back and
normalizer. The earlier
[native coordinate watch](SLOT_A_ONE_UP_NATIVE_COORDINATE_WATCH.md)
supplied the loaded U68 segment candidate, which this run checked again.
The separate p9 page-join note at commit `eeac65a`,
`docs/re/SLOT_A_POST_UP_TWO_PRINTER_PAGE_JOIN.md`, supplied the two endpoint
stops and EGA comparison. The direct stops below establish the writer
between those boundaries rather than assigning it from endpoint deltas.

## Input and loaded code

The ignored private replay used DOS v1.3 `START.EXE` SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
`GAME.OVR` SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
the 13,137-byte owner slot-A save SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`,
and the local heavy DOSBox-X binary SHA-256
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.
Media, RAM, frames, and screenshots remain ignored under
`resources/oracle-runs/slot-a-post-up-writer-pc-001/`.

At the reached clear-entry RAM stop, loaded U68
`2E60:073B..07F3` is 185 bytes, SHA-256
`04c0e7231a800b744bebd2d94ca11224d0d8d3e294fc6baf31d591a2f75cad7d`;
loaded U7D `2F6B:03D5..0479` is 165 bytes, SHA-256
`7bd8c6d35073b0d7101c4a3fbe2154e916a1f72424636636fcf0b48718f148a4`.
Both loaded spans match p9's independent clear-entry RAM. U68's loaded
span includes relocated far-call segment bytes, so its hash differs from
the original overlay hash. The reached U68 bit-set bytes at `+07DA..+07E4` are
`B0 01 50 B0 02 50 9A 25 00 B0 08`: push 1, push 2, call far
`08B0:0025`. The reached U7D word-store bytes at `+045D..+0461` are
`26 89 95 00 6E`, a word store to `ES:[DI+6E00]`.

## Same-run state and frame

Search completed at the saved `15,4 N 10:50` screen. The debugger was
paused at `1264:02FD`, breakpoints were armed, and exactly one Up key was
sent. The first post-Up printer return was `08DE:2B5A`,
`SS:SP=1D16:3C7C`. All five 640 KiB RAM captures below are from that
same resumed process. CPU words use the DOSBox order
`CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS`.

| Native stop | Reached CPU evidence | Party `DS:6AAD..6AAF` | Window-0 words `+18E,+190,+192` | RAM SHA-256 |
| --- | --- | --- | --- | --- |
| first printer return `08DE:2B5A` | `DS=14A0 SS:SP=1D16:3C7C` | 15,4,6 | 0,5,10 | `84619fbef1717b429a2ee8fb7a2bae828018161da86d9de43c0efe9c708a0b99` |
| U68 before far call `2E60:07E0` | `DS=14A0 ES=3A16 SS:SP=1D16:3DBA AX=0002` | 14,4,6 | 0,5,10 | `8dae80255e3157447e46abc7eb2990bb70d5e5690ece341505037363093b604b` |
| U7D after `+192` store `2F6B:0462` | `ES=3996 DI=9392 DX=000B` | 14,4,6 | 0,0,11 | `6d4fe6fbc100e90f93448b572716f76242dec1306f662a738fe0137121f26743` |
| U68 after U7D returns `2E60:07E5` | `SS:SP=1D16:3DBE` | 14,4,6 | 0,0,11 | `7b54b0b3549b4aa3d76894f6e67565f785ef86c70123066bc7de19749f4f4057` |
| row-15 clear entry `0F2D:00B7` | `SS:SP=1D16:3B62` | 14,4,6 | 0,0,11 | `b8fa241c1aed11cbcd1a6690e59fbf003d1719e7ce27f1a0e0fd9d641c270a45` |

At every stop, `DS:49D2=3996:0000` and
`DS:49D6=3A16:0000`. The live window-1 word at `+594` has bit 0
set, and the live limit table's `limit[2]` at `DS:363E` is 6.
Facing 6 reads `FF` from U68's X-step table at `DS:273C`, so the
8-bit party X step is `15 + (-1) = 14`.

At U68 `+07E0`, the first four bytes of the stack at `SS:3DBA` are
`02 00 01 00`, the pushed index 2 and count 1. Its saved caller frame
includes return `2E60:1031`, consistent with U68 entry 1's near call
at `+102E`. The far call reaches U7D `2F6B:03D5` with
`SS:SP=1D16:3DB6`. The entry stack begins
`E5 07 60 2E 02 00 01 00`: far return `2E60:07E5`, index 2,
count 1. This is the actual reached U68→U7D callback frame.

The U7D write-back loop stopped twice at the instruction after the
`+045D` store:

| Completed store | Live ES:DI and DX | Window-0 seven words immediately after |
| --- | --- | --- |
| `+190` | `3996:9390`, `DX=0000` | `[0,0,0,10,0,0,0]` |
| `+192` | `3996:9392`, `DX=000B` | `[0,0,0,11,0,0,0]` |

The store displacement wraps modulo 64 KiB:
`9390 + 6E00 = 0190` and `9392 + 6E00 = 0192`.
The merged U7D normalizer's arithmetic predicts the same values:
`5 + 1 = 6`, `6 - limit[2](6) = 0`, carry one into `10 + 1 = 11`.
The two saved 14-byte word windows and the five full RAM snapshots are
hash-locked by the replay audit.

## Verification and limits

Run `tools/replay/slot_a_post_up_state_writer_audit.py` with
`--run` pointing to the ignored replay directory and `--emulator` to
the audited heavy debugger binary. Optionally pass `--p9-run` for p9's
separate private page-join run. The audit validates executable/save hashes,
loaded code, the two live frames, five full RAM hashes, pointers, branch
bit, limit, the two word-store windows, and the normalization equation.
With p9 input it additionally checks both p9 endpoint hashes and confirms
all four 16,192-byte EGA plane windows are unchanged between its first
return and clear entry. The audit passed all checks. The two merged
synthetic CTests, `poolrad_overlay_u68_coordinate_writer_tests` and
`poolrad_overlay_u7d_entry1_tests`, cover their individual models. Both
passed (2/2) under `-Wall -Wextra -Wpedantic -Werror` in this worktree.

The p9 run's first-return and clear-entry RAM hashes are
`381d5bc520142c3c6db8d2712b309250b834121dcf7cf8a1f2dc0c6cd87c8774`
and
`54e1a3102c965595949559dc4d1bcf35334176c359cfd69b31a51d3f5c7b39af`.
Its party/window fields match this replay's corresponding stops, as
does the clear-entry `SS:SP=1D16:3B62`. The independent full-RAM
hashes differ, so no cross-run 640 KiB equality or full joined C
CPU/RAM parity is claimed. The write-back stops prove value-changing
stores in this run; they do not count same-value or transient writes.
This note does not integrate the state into the separate page-join C
route and earns zero additional strict full-function or game credit.

## Parity review

- **target_function:** the reached post-Up U68 coordinate store and
  U7D entry-1 `(1,2)` write-back through the row-15 clear boundary.
- **source_candidates:** the merged U68 coordinate-writer and U7D
  entry-1 models; the p9 page-join route supplies separate endpoints.
- **signals:** exact loaded branch/store bytes, direct same-run
  U68/U7D CPU and stack frames, live pointer/bit/limit values,
  two destination-specific native stores, five hash-locked RAM
  checkpoints, and independent p9 endpoint-field/EGA agreement.
- **score and verdict:** 5/5 ownership signals, **match** for the
  bounded native writer and argument mapping; joined C parity is
  **unclear** until its host callbacks and intervening state are
  composed and compared at a full CPU/RAM boundary.
- **review_notes:** p9's independent RAM hashes differ; the separate
  page route does not yet compute party/window state. The reached
  U68/U7D frame resolves the writer identity without assigning
  unrelated changes in the interval.
