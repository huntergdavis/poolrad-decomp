# Saved slot-A first-view printer inventory and page boundaries

In the two fresh saved-slot-A replays here, the route reaches **42** calls
to the loaded `RES_0709:0626` EGA printer before the actual `3582:3387`
first-view stop.
One early overlay call is followed by a 5,119-byte page change. A persistent
page then receives 23 calls. Another 410-byte page change precedes 18 more
calls over rows four through nine. The first 23-call page and the terminal
page have equal **32,000-byte values**, but they are distinct points in the
native route. This note does not claim a complete C frame or a new full
game-routine translation.

Before debugging, `deja "Poolrad slot A whole first view printer call chain
3582:3387 persistent EGA page inventory"` timed out without a reusable
result. This audit reuses the [bounded EGA printer page model](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md),
the [first title page evidence](SLOT_A_VIEW4_TITLE_PAGE.md), the
[row-nine composition](SLOT_A_ROW9_THREE_CALL_COMPOSITION.md), and the
[post-row-nine full-page suffix](SLOT_A_AFTER_ROW9_FULL_PAGE_SEQUENCE.md)
(PR #372). Each reached caller frame and page boundary here was checked in
fresh private replays.

## Pinned native route

The ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
the slot-A save is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`;
the private heavy DOSBox-X binary is
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.
The two ignored run directories are
`resources/oracle-runs/slot-a-ui-whole-first-view-p9-021/` and
`resources/oracle-runs/slot-a-ui-whole-first-view-p9-022/`.

The original expanded `RES_0709:0626` printer body is
`[0x76B6,0x7737)`, **129 bytes / 51 aligned instructions**, SHA-256
`b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`.
The loaded `0F2D:0626..06A6` body hashes to
`8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`.
Each resident caller is one five-byte far call to original `0709:0626`,
relocated to `0F2D:0626`; the original call SHA-256 is
`04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23`,
and the loaded call SHA-256 is
`cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`.
The earliest overlay call at `2A8E:17D0` has the same loaded five-byte call
and returns to `2A8E:17D5`; its original overlay file offset is not
established here.

| Native calls | Immediate far-return addresses, in order | Low-byte row / column / source length |
| --- | --- | --- |
| 1 | `2A8E:17D5` | 24 / 0 / 21 |
| 2–3 | `08DE:07A1`, `07BF` | 2 / 17 / 4; 2 / 33 / 6 |
| 4–6 | `08DE:0814`, `096D`, `09C3` | 4 / 17 / 14; 4 / 34 / 1; 4 / 37 / 2 |
| 7–9 | `08DE:1858`, `096D`, `09C3` | 5 / 17 / 6; 5 / 34 / 1; 5 / 38 / 1 |
| 10–12 | same three resident returns | 6 / 17 / 8; 6 / 34 / 1; 6 / 38 / 1 |
| 13–15 | same three resident returns | 7 / 17 / 12; 7 / 34 / 1; 7 / 37 / 2 |
| 16–18 | same three resident returns | 8 / 17 / 8; 8 / 34 / 1; 8 / 38 / 1 |
| 19–21 | same three resident returns | 9 / 17 / 7; 9 / 34 / 1; 9 / 38 / 1 |
| 22–24 | `08DE:2B5A`, `07A1`, `07BF` | 15 / 17 / 12; 2 / 17 / 4; 2 / 33 / 6 |
| 25–42 | `0814`, `096D`, `09C3`, then `1858`, `096D`, `09C3` repeated for rows 5–9 | rows 4–9, same columns and lengths as calls 4–21 |

The first call's `SS:SP` is `1D16:37AA`, with color 10; its private
length-plus-content source SHA-256 is
`3268e5ee46c67c0bb66ab90e8f2c5b34da5aa23c6ab52d5b19fb80ceba9071af`.
The audit checks the exact seven-word return/source/color/row/column frame
at every later call, the loaded and original far calls, the printer body,
EGA mode, `A000` page segments, and the live 1,416-byte font hash. It also
locks all first-cycle RAM and entry/return planes, all second-cycle entry
RAM, and the two-run boundary planes with aggregate SHA-256 values in the
replay script.

## Distinct page states and C evidence

Each page below concatenates four `A000:[0,0x1F40)` 8,000-byte planes.
The immediate call-24 return and call-25 entry pages were captured in
both runs and are byte-identical across runs.

| Stop | Full-page SHA-256 | Measured change from preceding boundary |
| --- | --- | ---: |
| call 1 immediate return | `1aeaaa19361512eb3937dae04daaef6bca7ff9723a6e1a2a01a95b203adafd6f` | within call 1 |
| call 2 entry, after intervening display work | `606d838123a21edb436339b6fbf4fb2583ae79b38964d89f24e763cdb34c7c77` | 5,119 |
| call 24 immediate return | `3eebca28aa66b05e4bfb4a1e199f01e2f8c849dd6a7dd82ed9df61af64a79738` | from 23 printer calls |
| call 25 entry, after intervening display work | `2aa65739f6bc280936e6203d6bb6cbaad26663475ee86e51468d48a4b7b4bfdf` | 410 |
| actual `3582:3387` after call 42 | `3eebca28aa66b05e4bfb4a1e199f01e2f8c849dd6a7dd82ed9df61af64a79738` | from second rendering |

From call 2's entry through call 24's return, every immediate native return
page equals the next call's entry page. A single persistent C page fed with
the independently decoded native source, font, and numeric arguments matches
**23/23 immediate native returns, 32,000/32,000 bytes each**. The C reports
sum to **101 cells, 3,232 modeled plane stores, and 1,754 net changed
values**. Calls 23–24 redraw row two without changing page values.

Starting at the separately measured call-25 entry page, the same C model
applied to 18 decoded second-cycle calls yields the actual terminal page
over **32,000/32,000 bytes** and reports 69 cells, 2,208 modeled stores,
and 410 net changed values. **The 18 immediate native returns were not
captured**, so this is terminal-only agreement. The non-printer page changes
before calls 2 and 25 are measured boundaries, not C-modeled effects.
Equal page values at call 24 and the terminal do not make call 24 the
terminal route. PR #372's three-call suffix page equality remains valid;
its capture did not count intervening printer entries. These replays expose
that missing call-count boundary. Full first-view C composition, including
both intervening display effects and per-return second-cycle parity,
remains open. The 42-call count is measured for these pinned replays and
should not be generalized to every timing or input route.

## Reproduce privately

Use `tools/oracle/prepare_run.sh` for each ignored run. At the load-slot
prompt, pause the heavy debugger with Alt+Pause, arm `BP 0F2D:0626` and
`BP 3582:3387`, resume, and select slot A. In the first run, record each
printer's `EV SS SP` and `MEMDUMPBIN 0000 0000 A0000`; at each immediate
return record all four EGA planes. In the second run, count each printer
entry, save call-24 return and call-25 entry pages, the call-25..42 entry
RAM, and the actual first-view page. For each plane use `OUTP 3CE 04`,
`OUTP 3CF <plane>`, `MEMDUMPBIN A000 0000 1F40`; restore read map 0 and
Graphics Controller selector 5 before `RUN`.

```sh
cmake -S . -B resources/build-slot-a-first-view-two-cycle \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-first-view-two-cycle --target \
  poolrad_slot_a_full_ega_printer_page_probe
ctest --test-dir resources/build-slot-a-first-view-two-cycle \
  --output-on-failure
python3 tools/replay/slot_a_first_view_two_cycle_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --first-run resources/oracle-runs/slot-a-ui-whole-first-view-p9-021 \
  --second-run resources/oracle-runs/slot-a-ui-whole-first-view-p9-022 \
  --probe resources/build-slot-a-first-view-two-cycle/poolrad_slot_a_full_ega_printer_page_probe
```

All original media, private text/font, RAM, EGA planes, screenshots, and
probe payloads remain under ignored `resources/`.
