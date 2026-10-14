# Saved view-four transfer and first later title-page range

The saved slot-A view-four tile path reaches `RES_0464:1B52` after its
tenth tile callback. The merged all-dirty transfer model predicts the entire
captured immediate `A200` staging and `A000` page union: **27,928/27,928
plane bytes and 168/168 metadata rows**. Among `A000:[0,0x2000)` bytes that
change *between that flush return and the saved first-view return*, the
lowest address is `A000:0291`. The first changed run is
`[0x0291,0x0295)`, screen byte row 16, columns 17 through 20, on each of
the four EGA planes. This is the lowest changed **address**, not a claim that
its store was the first page write in time.

A native write watch at `A000:0291` after the flush stops just after the
store at `RES_0709:0412`, with `CS:IP=0F2D:0415`. Its caller is the first
title printer at `RES_00BA:079C` in the merged `0767` field-list reader;
the far return is `08DE:07A1`. The saved branch chooses column 17 and row
2, with color 15 and a four-cell private Pascal source. A bounded page
projection through the already merged portable printer model matches the
immediate **32,768/32,768 native page plane bytes**, including 112 changed
bytes in those four cells. This adds no full-routine or game-identity credit.

Before debugging, `deja "Poolrad slot A view four callback 10 downstream
native video transfer dirty extents EGA page first unexplained byte"`
returned no exact indexed session. This work reused and checked the
[all-dirty transfer model](SLOT_A_U40C_ALL_DIRTY_TRANSFER_MODEL.md), the
[view-four placement path](SLOT_A_U3BD_VIEW4_PLACEMENT.md), the
[group-three plane model](SLOT_A_U40C_GROUP3_TILE_PLANE.md), the
[first clean glyph watch](SLOT_A_FIRST_CLEAN_PAGE_GLYPH_WRITE.md), the
[printer-page C model](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md), and the
[resident `0767` title-call note](RESIDENT_LOAD_PIECES_0767.md), and the
merged [PR #349 dirty-metadata model](SLOT_A_U40C_GROUP3_DIRTY_METADATA.md).
Its unmodified tenth callback changes zero metadata bytes because the
reached row extents are already broad enough.

## Hash-locked original and reached route

The ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Original image and file spans are distinguished below.

| Span | Source range | Bytes / aligned instructions | SHA-256 |
| --- | --- | ---: | --- |
| `RES_0464:1B52..1E5B` flush | START file `[0x61CB,0x64D5)` | 778; multi-path procedure | `7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c` |
| `RES_00BA:076D..07BE` title setup | expanded image `[0x130D,0x135F)` | 82 / 34 | `515cd8d55ed7ed32f3e0a1d710f972030cbc50816e13e2dfde8534684d345e43` |
| first title far call `+079C` | expanded image `[0x133C,0x1341)` | 5 / 1 | `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23` |
| `RES_0709:0626` printer | expanded image `[0x76B6,0x7737)` | 129 / 51 | `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed` |
| `RES_0709:01DF` glyph drawer | expanded image `[0x726F,0x767A)` | 1,035; reached store at `+0412` | `8d71dd4c0162462f99aef0fbbd4cfee472702f330bca1e07b50f192ffd972aef` |

In the first fresh saved-slot replay, the debugger stopped at the third
view-four placement `2D14:0AD5`, its tenth `2D14:02FB` tile callback,
and the placement return `2D14:0AD8`. A later stop at `0C88:1B52`
has return frame `2D14:05FB` and outer `2D14:0DAE`, video state 1, and
88 dirty rows `16..103`. Their page/stage union is `[0x03C3,0x1166)`.
The original flush-stage and page captures, all four planes at both
segments, exactly match the existing C model across 27,928 bytes.
It resets all 168 dirty-row metadata entries to their observed
post-flush values.

The first replay separately captured four full `A000:[0,0x2000)` planes
at immediate flush return and at `3582:3387` first-view return. The
four-plane lowest changed interval `[0x0291,0x0295)` is outside the
flush's dirty union. A second replay armed `BPM A000:0291` *after* its
flush return. The watch stopped at `0F2D:0415`, immediately after
`mov byte ptr es:[di],dl` at `+0412`; `ES=A000` and `DI=0291`.
Its retained frames return first to printer `0F2D:0694`, then resident
`08DE:07A1`. The address-only watch RAM SHA-256 is
`b82dafd31eed6cce9ab612a0fd2a80885efba66cb25ddc37ff91cc12b509e68c`.
The watch detects value changes at one address; it does not record all
same-value stores or order unrelated page writes.

The original title setup compares `DS:[49F3]` with zero at `+076D`.
Zero selects column 1; the saved nonzero branch selects column 17 at
`+077A`. It sets row 2 at `+077E`, copies the private title into a
stack Pascal buffer, then makes the far printer call at `+079C`.
The live printer arguments at `SS:3EAA` are that far source, color 15,
row 2, column 17. Its four-cell source and live 1,416-byte font hashes
are `4c1496b703656034f30aee34e197eb1f4abeff239d28dc9263af7a3df199090b`
and `fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
No title text or font bytes appear in this note or the audit.

## Same-run page parity and boundary

A third fresh replay stopped directly before `08DE:079C` and at its
immediate `08DE:07A1` return. Its four 8,192-byte pre-page plane hashes
match the first replay's full page state at flush return. Within this
third run, the printer targets four cells × eight scanlines × four
planes = **128 stores**. Exactly **112** plane bytes change, all in
screen rows 16..22, columns 17..20. The synthetic row-2 test uses four
invented glyphs and covers both resident column choices, repeated
same-value calls, untouched neighbors and invalid bounds. The new full
page probe uses the merged `pr_slot_a_printer_page_apply`; its output
matches all **32,768/32,768** native post-call plane bytes. The probe
change only parameterizes its existing private page window; its original
3,520-byte default is unchanged.

This proves one reached title-page contribution after the view-four
transfer. Other later clean-page writes and the complete static C frame
remain separate. The title source's meaning and unrelated UI state are
not inferred from its four-cell length.

```sh
cmake -S . -B resources/build-slot-a-view4-title-p5 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-view4-title-p5 --target \
  poolrad_slot_a_row2_title_page_tests \
  poolrad_slot_a_row2_title_page_probe \
  poolrad_slot_a_ega_all_dirty_probe
ctest --test-dir resources/build-slot-a-view4-title-p5 \
  -R '^poolrad_slot_a_row2_title_page_tests$' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/replay/slot_a_view4_title_page_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --transfer-run resources/oracle-runs/slot-a-view4-transfer-p5-001 \
  --title-run resources/oracle-runs/slot-a-view4-titleprint-p5-001 \
  --watch-run resources/oracle-runs/slot-a-view4-page0291-p5-001 \
  --transfer-probe \
    resources/build-slot-a-view4-title-p5/poolrad_slot_a_ega_all_dirty_probe \
  --title-probe \
    resources/build-slot-a-view4-title-p5/poolrad_slot_a_row2_title_page_probe
```

Original game files, saved-game media, private text, RAM, EGA planes and
screenshots stay under ignored `resources/`. The audit reports only
addresses, hashes, counts and parity totals.
