# Saved slot-A second title page effect

The saved first-view route reaches the second title printer call at
`RES_00BA:07BA` (`08DE:07BA`), immediately returning to `08DE:07BF`.
It draws six private cells at text row 2, columns 33–38, color 15.
The existing portable printer-page model predicts all **32,768/32,768**
captured immediate `A000` plane bytes. This is one bounded page effect;
it adds no full-C or game-identity credit.

Before debugging, `deja "Poolrad slot A view four top of screen row two
title next A000 page writer after RES_00BA 079C"` found no matching
indexed session. This audit reuses the [first title page and native
glyph watch](SLOT_A_VIEW4_TITLE_PAGE.md) (PR #351), the
[resident title-call path](RESIDENT_LOAD_PIECES_0767.md), the
[all-dirty transfer](SLOT_A_U40C_ALL_DIRTY_TRANSFER_MODEL.md), and the
[printer-page C model](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md).

## Native route and code bounds

The ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Expanded image `[0x1341,0x135F)`, corresponding to
`RES_00BA:07A1..07BF`, has **30 bytes / 14 aligned instructions** and
SHA-256
`958bd3fa3dc6a45c35d3db3f48dcb3339491da159ca4581988c5a4a59a19bd8d`.
The span starts by setting column `0x21`, pushes row/color/source,
copies its private Pascal source, and ends in the far printer call
`+07BA`. It contains no conditional branch. The five-byte original
call at image `[0x135A,0x135F)` hashes to
`04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23`;
its relocated target is `0F2D:0626`.

In a fresh saved-slot replay, the debugger stopped at `08DE:07BA`
with `SS:SP=1D16:3EAA`. The five argument words carry far source,
color, row, and column; their used low bytes are `15, 2, 33`.
The private source length is **6** and its length-plus-content hash is
`7039295c05fc9db0246086bb5dc3046e4ac8edffb2c6376a760c88f229dc7ef0`.
The live font's 1,416-byte hash is
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
`DS:[4A1E]=1` selects EGA and both page segment words are `A000`.
At `08DE:07BF`, the retained far return is `07BF:08DE` and the
printer's column byte has advanced to 39.

The loaded printer and glyph bodies match PR #351's hash-locked
`0F2D:0626` and `0F2D:01DF` spans. The glyph store is
`RES_0709:0412` (`0F2D:0412`), `mov byte ptr es:[di],dl`.
PR #351's native write watch independently observed that instruction
for the first title. This run's exact second-call before/after pages
and its loaded caller/printer/glyph code establish the same writer route;
it does not claim a separate second-call instruction watch.

## Four-plane page delta

The second call's complete `A000:[0,0x2000)` entry page is byte-identical
to PR #351's first title's immediate return page on all four planes.
From this second call's entry to its immediate return, **112** plane
bytes change, all inside its six cells, scanlines 16–22, columns
33, 34, 37, and 38. The lowest changed run is
`[0x02A1,0x02A3)` on each plane: global scanline 16, columns 33–34.
Six cells × eight scanlines × four planes give **192** modeled stores;
unchanged destinations are not counted as net changes by the snapshots.

| Saved stop | Four-plane 32,768-byte SHA-256 |
| --- | --- |
| `08DE:07BA` before second title | `6c3acbaa753288f479de94482ea6d93a7bd718128f1c41bd03f6fcac126987f1` |
| `08DE:07BF` immediate return | `2c5a4b2d34f737c37659ebe7ea39bd557603b6fbfdb7d051bc1a8bcaafc409e0` |
| `3582:3387` first-view return | `31b5f9b20ae4986dcefa3e11658e866f49211a35e85f731a9fce206b6337f41d` |

The same-run first-view page preserves all 112 changed bytes. Its first
four text rows (`A000:[0,0x500)`) exactly equal the immediate second-call
page. Relative to PR #351's saved view-four flush page, those rows have
**224** net changed plane bytes, all inside the first and second title
target rectangles. The later full page differs outside those rows;
this result does not establish a complete static frame. The resident
title path can be entered again before the first-view return; repeated
same-value stores are outside a snapshot delta.

Using the private source, live font, and second-call entry page, the
merged `pr_slot_a_printer_page_apply` reports six cells, 192 stores,
and 112 changed values. Its output matches **32,768/32,768** native
immediate page bytes. A synthetic test uses six invented glyphs at
columns 33–38, checks every page byte and untouched neighbors, repeat
behavior, the valid column-34 edge, and rejected column-35 overflow.

## Reproduce

The audit pins original and loaded code, all private RAM and plane
captures, the first-title handoff, the full C output, and top-row
persistence. Its outputs contain only addresses, hashes, and counts.

```sh
cmake -S . -B resources/build-slot-a-row2-second-p5 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-row2-second-p5 --target \
  poolrad_slot_a_row2_second_title_page_tests \
  poolrad_slot_a_row2_title_page_probe
ctest --test-dir resources/build-slot-a-row2-second-p5 \
  -R '^poolrad_slot_a_row2_second_title_page_tests$' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/replay/slot_a_row2_second_title_page_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --run resources/oracle-runs/slot-a-row2-second-title-p5-001 \
  --transfer-run resources/oracle-runs/slot-a-view4-transfer-p5-001 \
  --first-title-run resources/oracle-runs/slot-a-view4-titleprint-p5-001 \
  --probe resources/build-slot-a-row2-second-p5/poolrad_slot_a_row2_title_page_probe
```

To recapture, prepare a fresh hash-verified oracle run and pause at the
slot-A selection prompt. Set `BP 08DE:07BA` and `BP 08DE:07BF`,
resume, and capture full RAM and four `A000:[0,0x2000)` planes at each
stop. Restore Graphics Controller read map 0 and selector 5 before
resuming. The first-view return breakpoint is `3582:3387`. All original
game media, text, font, RAM, EGA pages and screenshots remain under
ignored `resources/`.
