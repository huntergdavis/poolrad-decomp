# Saved slot-A first view: row-nine seven-cell printer call

After [PR #361](https://github.com/huntergdavis/poolrad-decomp/pull/361)'s
row-eight column-38 return, the next observed page-changing call follows
`RES_00BA:0828 → 1816 → 1853 → RES_0709:0626`. It prints **seven**
cells at row **9**, columns **17–23**, color **11**, and returns to
`08DE:1858`. The existing bounded C printer-page model predicts the entire
immediate 14,080-byte page. This establishes parity for one reached call,
not a complete static frame or a new full-routine translation.

Before tracing, `deja "Poolrad slot A after row8 col38 printer next UI page
writer first view"` timed out without a reusable result. The merged
[column-38 page](SLOT_A_ROW8_COL38_PRINTER_PAGE.md),
[eight-cell route](SLOT_A_ROW8_EIGHT_CELL_PRINTER_PAGE.md), and
[printer model](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md) served as leads; this run
independently checks the live frame and immediate page.

## Original and loaded route

The ignored run is
`resources/oracle-runs/slot-a-text-ui-after-row8-col38-p9-015/`, prepared
with `tools/oracle/prepare_run.sh`. DOS v1.3 `START.EXE`, `GAME.OVR`, and
slot-A save SHA-256 values are respectively
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private heavy DOSBox-X binary SHA-256 is
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

| Expanded original image span | Bytes | SHA-256 | Meaning |
| --- | ---: | --- | --- |
| `[0x13B6,0x13CB)` | 21 | `18cf9b0353ef4afd377dce4e69f5ba20a64db1aa5e723eeb5611ff3ce3396b1d` | caller context `RES_00BA:0816..082B` |
| `[0x13C8,0x13CB)` | 3 | `101742b95d662bb42996389a5744d3371fb252318a3e91c03de2d6e0f01469d3` | near call `0828 → 1816` |
| `[0x23B6,0x2432)` | 124 | `645caa0e81651dc9b2be1f156ea343d47f83a195f64f84a5c6fd828a89f50175` | reached 49-instruction `1816` prefix through `RETF 0x0A` |
| `[0x23F3,0x23F8)` | 5 | `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23` | far call `1853 → RES_0709:0626` |
| `[0x76B6,0x7737)` | 129 | `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed` | bounded printer body |

The corresponding loaded spans `08DE:0816..082B`, `08DE:0828..082B`,
`08DE:1816..1892`, `08DE:1853..1858`, and `0F2D:0626..06A7` hash to
`18cf9b0353ef4afd377dce4e69f5ba20a64db1aa5e723eeb5611ff3ce3396b1d`,
`101742b95d662bb42996389a5744d3371fb252318a3e91c03de2d6e0f01469d3`,
`ea633c656758bfd81c77f802f1ba830ede8f0973339f67d57a2880d3413d9d01`,
`cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`,
and `8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`.
The original far call targets `0709:0626`; its loaded target is
`0F2D:0626`.

The prior row-eight return at `08DE:09C3` has parent frame `SS:3EA4`
passing row 8, column 38, and source `4513:0008`. At the next printer
entry, `SS:3E92` gives return `08DE:1858`, private source `4529:0008`,
color 11, row 9, and column 17. Its `1816` parent frame at `SS:3EA4`
returns to `08DE:082B`, passes zero override, and passes the same row,
column, and source pointer. Source fields `+0x10D=1` and `+0x10E=0`
select color 11 in the reached `1816` branch. The Pascal source length is
**7**; its length-plus-content SHA-256 is
`df900d38b6c327a4530a20ce19d2cb15b823e82694f232ef6e448d05f22c89e3`.
The live 1,416-byte font SHA-256 is
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
`DS:4A1E=1` selects EGA, and both page segments are `A000`.

## Same-run page contribution

Each page concatenates four 3,520-byte EGA planes from
`A000:[0x03C0,0x1180)`. The previous `08DE:09C3` page and the next
`0F2D:0626` entry page are byte-identical. These snapshots cannot exclude
intervening writes later restored.

| Stop | 14,080-byte page SHA-256 |
| --- | --- |
| `08DE:09C3` and next `0F2D:0626` entry | `dfd1c5681f895d4c5495ef166bc955db0a258d569dd19cc72fcbef56eac55b64` |
| `08DE:1858` immediate printer return | `01195f7efda5f563bf20ef0b3d97f687786cd4b6585f89f30ed5fc494f5dff9b` |
| `3582:3387` first-view return | `d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b` |

Seven cells × eight scanlines × four planes gives **224** modeled
plane-byte stores. Immediate snapshots differ at **147** positions:
columns 17–23 on global scanlines 72–78 in planes 0, 1, and 3. Plane 2
and scanline 79 have no net value change. All 147 values persist at
first-view return. Another **28** page bytes differ after this return;
their writers are unassigned. The total first-view difference from this
call's entry is **175** bytes. Snapshot deltas do not count same-value
stores.

Given this run's ignored source, font, and entry page,
`pr_slot_a_printer_page_apply` reports seven cells, 224 stores, and 147
changed values. Its output matches **14,080/14,080** native immediate
post-call page bytes. The public synthetic test uses seven invented glyphs
and an invented page to check placement, plane selection, neighbors,
repeat behavior, and bounds. No game text, font bits, RAM, image, or page
payload is tracked.

## Reproduce locally

Prepare a fresh ignored run and start the pinned heavy DOSBox-X with
`DEBUGBOX START`. At the title press Return; at the main menu press `l`;
at `LOAD WHICH GAME` press `a`. Arm `BP 08DE:09C3`; select the fifth hit
whose parent frame gives row 8 and column 38 and whose page SHA-256 is
`dfd1c5681f895d4c5495ef166bc955db0a258d569dd19cc72fcbef56eac55b64`.
Arm `BP 0F2D:0626` for the next printer entry, then `BP 08DE:1858`
for its immediate return. Use `BPDEL *` and `BP 3582:3387` for
first-view return. At each stop, `MEMDUMPBIN 0000 0000 A0000` records
private RAM. For each EGA plane, select Graphics Controller register 4
with `OUTP 3CE 04`, select read map 0..3 with `OUTP 3CF`, and run
`MEMDUMPBIN A000 03C0 DC0`. Restore read map 0 and selector 5 before
resuming. Keep all dumps under ignored `resources/`.

```sh
cmake -S . -B resources/build-slot-a-row9-seven \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-row9-seven --target \
  poolrad_slot_a_row9_seven_cell_printer_tests poolrad_slot_a_printer_page_probe
ctest --test-dir resources/build-slot-a-row9-seven \
  -R '^poolrad_slot_a_row9_seven_cell_printer_tests$' --output-on-failure
python3 tools/replay/slot_a_row9_seven_cell_printer_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --run resources/oracle-runs/slot-a-text-ui-after-row8-col38-p9-015 \
  --probe resources/build-slot-a-row9-seven/poolrad_slot_a_printer_page_probe
```

The audit hash-locks original and loaded call/body spans, caller frames,
source, font, every captured page plane, first-view persistence, and exact
C output. Probe records remain in the ignored run.
