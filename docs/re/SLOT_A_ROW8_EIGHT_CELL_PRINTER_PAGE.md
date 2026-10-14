# Saved slot-A first view: row-eight eight-cell printer call

After [PR #354](https://github.com/huntergdavis/poolrad-decomp/pull/354)'s
row-seven column-37 return, the next reached page-changing call is
`RES_00BA:0828 → 1816 → 1853 → RES_0709:0626`. It prints **eight** cells
at row **8**, columns **17–24**, color **11**, and returns to `08DE:1858`.
The existing bounded C printer-page model predicts the entire immediate
14,080-byte page. This is parity for one reached call, not a complete static
frame or a new full-routine translation.

Before tracing, `deja "Poolrad slot A after row7 col37 two cell printer next UI
page writer first view"` returned no focused session. The shorter
`deja "slot A UI page writer"` returned unrelated sessions. The merged
[PR #354 page](SLOT_A_ROW7_COL37_TWO_CELL_PRINTER_PAGE.md) and the bounded
[printer model](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md) were used as leads;
this run independently checks the live path, frame, and immediate page.

## Original and loaded route

The ignored run is
`resources/oracle-runs/slot-a-text-ui-after-col37-row7-p9-012/`, prepared
with `tools/oracle/prepare_run.sh`. DOS v1.3 `START.EXE`, `GAME.OVR`, and
slot-A save SHA-256 values are respectively
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private heavy DOSBox-X binary SHA-256 is
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

| Expanded original image span | Bytes | SHA-256 | Meaning |
| --- | ---: | --- | --- |
| `[0x13B6,0x13CB)` | 21 | `18cf9b0353ef4afd377dce4e69f5ba20a64db1aa5e723eeb5611ff3ce3396b1d` | outer argument and call path `RES_00BA:0816..082B` |
| `[0x13C8,0x13CB)` | 3 | `101742b95d662bb42996389a5744d3371fb252318a3e91c03de2d6e0f01469d3` | near call `RES_00BA:0828 → 1816` |
| `[0x23B6,0x2432)` | 124 | `645caa0e81651dc9b2be1f156ea343d47f83a195f64f84a5c6fd828a89f50175` | bounded `RES_00BA:1816` body through `RETF 0x0A` |
| `[0x23F3,0x23F8)` | 5 | `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23` | far call `RES_00BA:1853 → RES_0709:0626` |
| `[0x76B6,0x7737)` | 129 | `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed` | bounded printer body |

The corresponding loaded spans `08DE:0816..082B`, `08DE:0828..082B`,
`08DE:1816..1892`, `08DE:1853..1858`, and `0F2D:0626..06A7` hash to
`18cf9b0353ef4afd377dce4e69f5ba20a64db1aa5e723eeb5611ff3ce3396b1d`,
`101742b95d662bb42996389a5744d3371fb252318a3e91c03de2d6e0f01469d3`,
`ea633c656758bfd81c77f802f1ba830ede8f0973339f67d57a2880d3413d9d01`,
`cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`,
and `8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`.
The original far call targets `0709:0626`; its loaded target is `0F2D:0626`.

At the anchor `08DE:08BA`, the loop frame word at `SS:3EBC` is `0x1107`
(row 7, column 17), and the EGA page has PR #354's immediate-return hash.
The next printer entry has `SS:SP=1D16:3E92`. Its frame gives return
`08DE:1858`, source `4513:0008`, color 11, row 8, and column 17. The
`1816` parent frame returns to `08DE:082B`, passes zero at `BP+6`, and
passes the same row, column, and source pointer. Source fields `+0x10D=1`
and `+0x10E=0` select color 11 in the bounded `1816` body. The Pascal
source length is **8**; its length-plus-content SHA-256 is
`571899d9b8851d7601f2c6f985a0af1dd75f5788993877c71107d3c27bdf86be`.
The live 1,416-byte font SHA-256 is
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
`DS:4A1E=1` selects EGA, and both page segments are `A000`.

## Same-run page contribution

Each page concatenates four 3,520-byte EGA planes from
`A000:[0x03C0,0x1180)`. The row-seven `08DE:08BA` page and the next
`0F2D:0626` entry page are byte-identical. These snapshots cannot exclude
intervening writes later restored.

| Stop | 14,080-byte page SHA-256 |
| --- | --- |
| `08DE:08BA` and next `0F2D:0626` entry | `99ce3c3496cf59d0f0e43b67ac66d03ad0e09af30168d19ea0bf55877500cbc0` |
| `08DE:1858` immediate printer return | `8723a10b9ac3747fd9edc42003a59c2434d1aa734fab75db17483eebd0b95498` |
| `3582:3387` first-view return | `d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b` |

Eight cells × eight scanlines × four planes gives **256** modeled
plane-byte stores. Immediate snapshots differ at **168** positions:
columns 17–24 on global scanlines 64–70 in planes 0, 1, and 3.
Plane 2 and scanline 71 have no net value change. All 168 values persist
at first-view return. Another **203** page bytes differ after this return;
their writers are unassigned. The total first-view difference from this
call's entry is **371** bytes. Snapshot deltas do not count same-value stores.

Given this run's ignored source, font, and entry page,
`pr_slot_a_printer_page_apply` reports eight cells, 256 stores, and 168
changed values. Its output matches **14,080/14,080** native immediate
post-call page bytes. The public synthetic test uses eight invented,
distinct glyphs and an invented page to check placement, plane selection,
neighbors, repeat behavior, and bounds. No game text, font bits, RAM,
image, or page payload is tracked.

## Reproduce locally

Start the pinned heavy DOSBox-X with the prepared ignored run. At the
code-word prompt press Return; at the main menu press `l`; at `LOAD WHICH
GAME` press `a`. The long `AUTOTYPE -w` delay is capped, so use visible
screen gates for those keys. Arm `BP 08DE:08BA`; its first four hits have
row bytes 4, 5, 6, and 7. Select the row-seven hit whose sampled page SHA-256
is `99ce3c3496cf59d0f0e43b67ac66d03ad0e09af30168d19ea0bf55877500cbc0`.
Clear that breakpoint, arm `BP 0F2D:0626`, and capture the next entry.
Arm `BP 08DE:1858` for its immediate return, then `BP 3582:3387` for
first-view return. At each stop, `MEMDUMPBIN 0000 0000 A0000` records
private RAM. For each EGA plane, select Graphics Controller register 4
with `OUTP 3CE 04`, select read map 0..3 with `OUTP 3CF`, and run
`MEMDUMPBIN A000 03C0 DC0`. Restore read map 0 and selector 5 before
resuming. Keep all dumps under ignored `resources/`.

```sh
cmake -S . -B resources/build-slot-a-row8-eight \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-row8-eight --target \
  poolrad_slot_a_row8_eight_cell_printer_tests poolrad_slot_a_printer_page_probe
ctest --test-dir resources/build-slot-a-row8-eight \
  -R '^poolrad_slot_a_row8_eight_cell_printer_tests$' --output-on-failure
python3 tools/replay/slot_a_row8_eight_cell_printer_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --run resources/oracle-runs/slot-a-text-ui-after-col37-row7-p9-012 \
  --probe resources/build-slot-a-row8-eight/poolrad_slot_a_printer_page_probe
```

The audit hash-locks original and loaded call/body spans, caller frames,
source, font, every captured page plane, first-view persistence, and
exact C output. Probe records remain in the ignored run.
