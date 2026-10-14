# Saved slot-A first view: persistent second printer-cycle C page

This bounded C composition starts with **one** measured native page at
printer call 25 entry, then applies 18 portable C printer calls and five
bounded zero-row effects to the same four-plane page. It matches every
native call entry and immediate return: **36/36 checkpoints,
32,000/32,000 bytes each**. Later native pages are comparison targets only;
none is copied into the C state. The result is second-cycle page parity,
not a complete first-view C frame or a full translation of either resident
rectangle routine.

The earlier [independent per-call audit and first row-five write watch](SLOT_A_SECOND_CYCLE_IMMEDIATE_RETURNS.md)
established the call-27 → call-28 zero clear and its first value-changing
guest instruction. Before this new trace, `deja "Poolrad slot A second-cycle
row boundary RES_03F1 0137 RES_0709 00B7 clears rows 6 through 9 persistent
EGA page composition"` timed out at 20 seconds; no indexed result was reused.
The existing C printer model and that previous watch were used as leads.

## Four more reached clear requests

The fresh ignored run `slot-a-ui-second-cycle-callers-p9-025` uses the
same DOS v1.3 START, GAME, slot-A save, and private heavy debugger hashes
listed in the preceding note. At the seventh `08DE:09C3` printer return
after slot-A load, the debugger replaced that breakpoint with
`BP 0C15:01F1`. The next five successive hits were the row-five through
row-nine EGA rectangle calls. Each paused instruction is the loaded far
call from `RES_03F1:0137` to `RES_0709:00B7`; each enclosing frame returns
to `RES_00BA:07EF` from its call at `+07EA`. At all five stops,
`DS:[4A1E]=1` and page segments `DS:[4A16]` and `DS:[4A18]` are `A000`.

| Reached clear | Caller low-byte arguments `(top, right, bottom, left)` | 640 KiB RAM SHA-256 |
| --- | --- | --- |
| row 5 | `(5,38,5,17)` | `a6c7ba2f84683a6d2db0fac17e91118cc397884de9eeeb07f3c435608347d0c2` |
| row 6 | `(6,38,6,17)` | `86603906a03793a87900b95b1b6fc2fbd632462781435703aa935abac6055633` |
| row 7 | `(7,38,7,17)` | `28c6b8b215d8e6b7c196a4647d65622185351d3197e353d64f26267fe8ac7539` |
| row 8 | `(8,38,8,17)` | `02233c08911761f46c243f6b2d2b278d919da18c2523c7c5727f46fcdbb0e66b` |
| row 9 | `(9,38,9,17)` | `d162a2ff5b0a8d773dc7f7c53cfee297a1a06758e9eb1ac4290787348e8838a8` |

The five entry RAM images independently verify the original and loaded
`RES_03F1:0137` body and its far call, the original and loaded
`RES_0709:00B7` body, and the original/loaded `RES_00BA:07EA` far call.
The original expanded-image hashes are `1258f9ea251527a82be56c60ecbd1f07e300c994bc0fdbac2a6537369080f5fc`
for `[0x4047,0x41F8)`, `f26747c1e8836399f118763dfee5a2bdeace551495f75b5533772b888fa65ff5`
for `[0x7147,0x7201)`, `2286767d9eccf2e3fe83669ef5367f5ef97da8c2145e12d54b0291510e19093c`
for the rectangle call `[0x4101,0x4106)`, and
`395783ec4ad75a0552ed076373c516213f522e5aacc77c12d8fbf9ead76dd23f`
for the outer call `[0x138A,0x138F)`. Their loaded hashes and bounds
are checked by the audit.

At the final row-nine caller stop, a separate breakpoint confirmed entry
to the far callee at `0F2D:00B7`. Its `SS:SP=1D16:3E94` frame returns to
`0C15:01F6` and carries low-byte arguments
`(color=0, page=0, bottom=9, right=38, top=9, left=17)`.
That 640 KiB entry RAM SHA-256 is
`672e433c28362f3445af87c4a04c1c68b1d4cacd407fa958cbcc616f9e2153b5`.
The prior row-five watch directly identified the zero writer
`131C:16B7`. Rows six through eight have caller-entry and byte-verified
call-target evidence plus measured page deltas; they do not have separate
per-byte write watches. Snapshot parity does not count same-value or
transient hardware writes.

## Bounded C effect and persistent result

`pr_slot_a_second_zero_row_apply` accepts only the observed EGA tuple:
color zero, one of rows 5..9, inclusive byte columns 17..38, and a
supplied page span containing every target. It clears eight scanlines
and four planes, reporting **704 plane-byte targets per row**; any
unsupported tuple or insufficient span returns a boundary status before
changing the page. Synthetic tests check targeting, untouched neighbors,
repeat same-value behavior, and rejected inputs. This helper models only
the reached page effect of `RES_03F1:0137 → RES_0709:00B7`.

The private composition audit verifies each reached caller frame and
supplies its row argument to the C clear helper. The C probe begins with
call 25's native entry page, SHA-256
`2aa65739f6bc280936e6203d6bb6cbaad26663475ee86e51468d48a4b7b4bfdf`.
It retains that page while applying the five clears before printer calls
28, 31, 34, 37, and 40. Its complete 36-page entry/return output stream
is **1,152,000 bytes**, SHA-256
`650fde0e71c40848f90d50f48fe2738d1049a333a3b244f93fc174c1794a9748`,
equal to the aggregate of the independent native captures. Every
checkpoint is compared separately, so an intermediate discrepancy cannot
be hidden by the terminal page.

Each printer's source text and call arguments also come from its captured
native entry RAM. C production of those requests, the font, and the
inter-cycle reset is outside this bounded page-composition result.

The C reports **69 printer cells, 2,208 printer plane-byte stores, 1,404
summed per-call printer value changes**, and **five clears, 3,520 clear
plane-byte targets, 994 clear value changes**. The five clear changes
split as 154, 196, 273, 196, and 175, matching the five native
inter-call deltas. The first-cycle page, 410-byte inter-cycle reset, and
other first-view effects remain separate boundaries.

## Local reproduction

The focused synthetic test and full strict CTest suite passed **1/1** and
**178/178** respectively in the local build below. The private audit passed
all 36 page checkpoints.

```sh
cmake -S . -B resources/build-slot-a-second-cycle-local \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-second-cycle-local --target \
  poolrad_slot_a_second_cycle_zero_row_tests \
  poolrad_slot_a_second_cycle_compose_probe \
  poolrad_slot_a_full_ega_printer_page_probe
ctest --test-dir resources/build-slot-a-second-cycle-local \
  -R '^poolrad_slot_a_second_cycle_zero_row_tests$' --output-on-failure
python3 tools/replay/slot_a_second_cycle_compose_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --run resources/oracle-runs/slot-a-ui-second-cycle-p9-023 \
  --watch resources/oracle-runs/slot-a-ui-second-cycle-watch-p9-024 \
  --caller-run resources/oracle-runs/slot-a-ui-second-cycle-callers-p9-025 \
  --printer-probe resources/build-slot-a-second-cycle-local/poolrad_slot_a_full_ega_printer_page_probe \
  --compose-probe resources/build-slot-a-second-cycle-local/poolrad_slot_a_second_cycle_compose_probe
```

All original DOS binaries, save, RAM, text, font, EGA page files, probe
inputs, and probe outputs remain under ignored `resources/`. The public
model, synthetic test, audit, and note contain only bounded logic and
address/hash/count/instruction evidence.
