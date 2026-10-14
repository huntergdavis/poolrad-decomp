# Saved slot-A first view: row-nine three-call page composition

The saved slot-A native replay reaches three `RES_0709:0626` EGA printer
calls in this order: `RES_00BA:0828 → 1816 → 1853` for row 9 columns
17–23, `RES_00BA:0875 → 08EF → 0968` for column 34, and
`RES_00BA:08B7 → 0973 → 09BB → 1037 → 09BE` for column 38. Their
return addresses are `08DE:1858`, `08DE:096D`, and `08DE:09C3`.
Starting with the **same-run** page before the first call, three
sequential applications of the existing bounded C page model match each
immediate native return over all **14,080 page bytes**. The final C page
also matches the same-run first-view return over **14,080/14,080 bytes**.
This is parity for these three page effects; it is not a complete frame,
whole-view renderer, or new full-routine translation.

Before implementation, `deja "Poolrad slot A row nine seven cell col17
col34 col38 C printer composition first-view page"` timed out without a
reusable session result. The merged [seven-cell](SLOT_A_ROW9_SEVEN_CELL_PRINTER_PAGE.md),
[column-34](SLOT_A_ROW9_COL34_PRINTER_PAGE.md), and
[column-38](SLOT_A_ROW9_COL38_PRINTER_PAGE.md) audits supplied leads and
hashes; this audit independently checks the original and loaded route,
live frames, and private inputs in a fresh single run.

## Native evidence and C comparison

The ignored run is
`resources/oracle-runs/slot-a-text-ui-row9-composition-p9-018/`, prepared
with `tools/oracle/prepare_run.sh`. DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
the slot-A save is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`;
the heavy DOSBox-X binary is
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.
The audit hash-locks seven 640 KiB RAM captures and every 3,520-byte
EGA plane. Original and loaded call/body span hashes are in the three
linked notes and verified again by the composition audit. The original
far target `0709:0626` relocates to loaded `0F2D:0626`; the bounded
129-byte original printer body at expanded `[0x76B6,0x7737)` hashes to
`b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`.

| Call | Entry frame at SS:SP | Source length SHA-256 | Color, row, column | Immediate changed bytes |
| --- | --- | --- | --- | ---: |
| 1 | `1D16:3E92`, return `08DE:1858` | `df900d38b6c327a4530a20ce19d2cb15b823e82694f232ef6e448d05f22c89e3` | 11, 9, 17 | 147 |
| 2 | `1D16:3C96`, return `08DE:096D` | `9057f89eb79a4aa2452527d567cb057bbdcd4d23c5e6cc2039bc3bc61f1230c6` | 10, 9, 34 | 14 |
| 3 | `1D16:3D94`, return `08DE:09C3` | `17333b15b4a5afd16cac55a104b554fc63cc8731c76249dddce5ec3363fe4f38` | 10, 9, 38 | 14 |

The source hashes cover Pascal length plus content, not just text. The
live 1,416-byte font hashes to
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`;
`DS:4A1E=1` selects EGA, and both page segments are `A000` at all
three entries. Parent frames identify the three distinct resident caller
paths. The call-one node branch selects color 11; the call-three node
bytes `+0x11B` and `+0x32` both equal 6 and select color 10.

Each page is four 3,520-byte EGA planes from
`A000:[0x03C0,0x1180)`, concatenated in plane order. The first return
equals the second entry; the second return equals the third entry; the
third return equals the same-run first-view page. These are bytewise
snapshot equalities, which do not exclude intervening writes that were
later restored.

| Stop | Full page SHA-256 |
| --- | --- |
| Call 1 entry | `dfd1c5681f895d4c5495ef166bc955db0a258d569dd19cc72fcbef56eac55b64` |
| Call 1 return and call 2 entry | `01195f7efda5f563bf20ef0b3d97f687786cd4b6585f89f30ed5fc494f5dff9b` |
| Call 2 return and call 3 entry | `4abd88a43973afe33f43c772d6e7988cf83da3106159d802490f14db73dd32d6` |
| Call 3 return and first-view return | `d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b` |

The three C calls report **9 cells, 288 plane stores, and 175 net
changed byte values**: 147 at columns 17–23, then 14 at column 34,
then 14 at column 38. The changed sets are disjoint. Same-value or
transient guest writes are not counted by page snapshots. The C page
state is carried from one call to the next; no expected page or tuple is
assigned. The synthetic test uses invented glyphs and checks the full
four-plane page after each step, including untouched bytes.

## Reproduce locally

Use a fresh ignored run with the pinned slot-A save and heavy DOSBox-X.
`DEBUGBOX START`; at the title press Return, at the main menu press `l`,
and at `LOAD WHICH GAME` press `a`. On the fifth `08DE:09C3` hit for
the prior row-eight column-38 call, arm `BP 0F2D:0626`. Capture the
first row-nine entry at `0F2D:0626`, its `08DE:1858` return, the next
`0F2D:0626` entry and `08DE:096D` return, and the next `0F2D:0626`
entry and `08DE:09C3` return. Then capture `3582:3387` first view.
At each stop, `MEMDUMPBIN 0000 0000 A0000` captures private RAM.
For each EGA plane 0–3, `OUTP 3CE 04`, `OUTP 3CF <plane>`, and
`MEMDUMPBIN A000 03C0 DC0` capture its private page span. Restore
read map 0 and Graphics Controller selector 5 before each `RUN`. Keep
all dumps and probe records under ignored `resources/`.

```sh
cmake -S . -B resources/build-slot-a-row9-composition \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-row9-composition --target \
  poolrad_slot_a_row9_three_call_composition_tests \
  poolrad_slot_a_printer_page_probe
ctest --test-dir resources/build-slot-a-row9-composition \
  -R '^poolrad_slot_a_row9_three_call_composition_tests$' --output-on-failure
python3 tools/replay/slot_a_row9_three_call_composition_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --run resources/oracle-runs/slot-a-text-ui-row9-composition-p9-018 \
  --probe resources/build-slot-a-row9-composition/poolrad_slot_a_printer_page_probe
```

The private audit verifies hashes, original and loaded call targets,
parent and printer frames, source lengths, color branch inputs, EGA
mode, geometry of each native delta, and exact sequential C outputs.
It emits addresses, hashes, and counts only. No game bytes, source text,
font, page, save, or screenshot is tracked.
