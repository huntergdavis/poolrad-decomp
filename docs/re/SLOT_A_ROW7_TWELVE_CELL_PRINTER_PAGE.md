# Saved slot-A first view: row-seven twelve-cell printer call

After [PR #345](https://github.com/huntergdavis/poolrad-decomp/pull/345)'s
row-six column-38 printer return, the next reached page-changing call is
`RES_00BA:0828 → RES_00BA:1816 → RES_0709:0626`. Its entry page equals
PR #345's return page. The nested printer draws **12** cells at row **7**,
column **17**, changing **231** EGA plane bytes. The existing bounded C
printer-page model predicts the entire immediate 14,080-byte page. This is
parity for one reached call, not a complete static frame or a new full-routine
translation.

Before tracing, `deja "Poolrad slot A after RES_00BA 09BE row6 column38 printer
next reached page-changing call 644 bytes"` was invoked and returned no focused
session. PR #345's page hash, [PR #340](https://github.com/huntergdavis/poolrad-decomp/pull/340)'s
`RES_00BA:1816` call path, and the merged
[printer model](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md) were reused as leads. The
route, frame, and page result below were measured in a fresh native run.

## Original and loaded route

The ignored run is `resources/oracle-runs/slot-a-text-ui-after-col38-p9-009/`,
prepared with `tools/oracle/prepare_run.sh`. DOS v1.3 `START.EXE`, `GAME.OVR`,
and slot-A save SHA-256 values are respectively
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private heavy DOSBox-X binary SHA-256 is
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

| Expanded original image span | Bytes | SHA-256 | Meaning |
| --- | ---: | --- | --- |
| `[0x138A,0x138F)` | 5 | `395783ec4ad75a0552ed076373c516213f522e5aacc77c12d8fbf9ead76dd23f` | `RES_00BA:07EA` far call to `RES_03F1:0137` |
| `[0x4047,0x41F8)` | 433 | `1258f9ea251527a82be56c60ecbd1f07e300c994bc0fdbac2a6537369080f5fc` | `RES_03F1:0137` through `RETF 8` |
| `[0x13C8,0x13CB)` | 3 | `101742b95d662bb42996389a5744d3371fb252318a3e91c03de2d6e0f01469d3` | `RES_00BA:0828` near call to `1816` |
| `[0x23B6,0x2492)` | 220 | `47a999a7d5c19202877fb35003099aab45dac77ce4bb95e93d132020a2373918` | bounded `RES_00BA:1816` body |
| `[0x23F3,0x23F8)` | 5 | `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23` | `RES_00BA:1853` far call to `RES_0709:0626` |
| `[0x76B6,0x7737)` | 129 | `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed` | bounded printer body |

Loaded spans at `08DE:07EA..07EF`, `0C15:0137..02E8`,
`08DE:0828..082B`, `08DE:1816..18F2`, `08DE:1853..1858`, and
`0F2D:0626..06A7` hash respectively to
`2f89ade8cd970b19f81e674324278b7e2f7845bd119dbf92374051d645fc1850`,
`f50a7fde3dcc5886f55beb81c5fcdc22efe4348b52d8df35c70a646d5a309d5a`,
`101742b95d662bb42996389a5744d3371fb252318a3e91c03de2d6e0f01469d3`,
`35cdb1f71e2bc52f5f2d377d647594481f50921a7c09a0aed1b01057aa6087ac`,
`cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`,
and `8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`.
The two loaded far calls target `0C15:0137` and `0F2D:0626`.

At the row-seven `08DE:07EA` call, the next node pointer is `44F5:0008`.
The `0C15:0137` entry frame carries four low-byte arguments in stack order
`(7,38,7,17)` and returns to `08DE:07EF`. The sampled EGA page is unchanged
at that return. The subsequent `08DE:0828` call enters `1816` with return
`08DE:082B`, row 7, column 17, and node pointer `44F5:0008`. Its nested
printer entry has `SS:SP=1D16:3E92`, return `08DE:1858`, source
`44F5:0008`, color 11, row 7, and column 17. The Pascal source length is
12; its length-plus-content SHA-256 is
`b7be7afb7653c61771446859f1043ab2cd56400f35d7a097ad5dd62271f7f9c5`.
The live 1,416-byte font SHA-256 is
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
`DS:4A1E=1` selects EGA, and both page segments are `A000`.

## Same-run page contribution

Each page concatenates four 3,520-byte EGA planes from
`A000:[0x03C0,0x1180)`. The first stop is the exact post-PR #345 page. The
intermediate page snapshots are equal; they cannot exclude intervening writes
later restored.

| Stop | 14,080-byte page SHA-256 |
| --- | --- |
| `08DE:07EA` entry, `08DE:07EF` return, `08DE:1816` entry | `a6419841fb5aa3e5a40a401c081b69be718b692af57dd3a3d29b58e03ed8158d` |
| `08DE:1858` printer return and `08DE:082B` parent return | `bd641160cb016bb6ff8ec5043235c972ae829e910a589afc3618ea7b7ab34f6f` |
| `3582:3387` first-view return | `d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b` |

Twelve cells × eight scanlines × four planes gives **384** modeled
plane-byte stores. Immediate snapshots differ at **231** positions: 77 each
in planes 0, 1, and 3, on scanlines 32–38 of this page and columns 17–23
and 25–28. Plane 2 has no net changes. All 231 values persist at first-view
return. Another **413** page bytes differ after the printer return; their
writers are unassigned. Snapshot deltas do not count same-value stores.

Given this run's ignored source, font, and entry page,
`pr_slot_a_printer_page_apply` reports 12 cells, 384 stores, and 231 changed
values. Its output matches **14,080/14,080** native immediate post-call page
bytes. The public synthetic test uses an invented glyph and page to check
targeting, neighbors, repeat behavior, and bounds. The ignored paired native/C
visual covers **320×88** pixels with **28,160/28,160** pixel indices equal;
its diff has zero mismatches. The paired and diff PNG SHA-256 values are
`b22b3c17989e552a769454f2600c1e7c11b3bc5168a963581f87d518587033a5`
and `d7635b6676e70e19a35a69d1bc5083906c475d39693612e1ede8d5001aea5eca`.
Those private renderings do not establish a complete C static-frame result.
The `RES_03F1:0137` zero delta is confined to this sampled page; other
side effects were not classified here.

## Reproduce locally

The first-view loop reuses `08DE:07EA`, `08DE:0828`, and `08DE:1816`.
Anchor this invocation with the prior `08DE:09C3` row-six column-38 return
page SHA `a6419841fb5aa3e5a40a401c081b69be718b692af57dd3a3d29b58e03ed8158d`,
then require row 7 and column 17 in the `1816` frame. Capture the next
`08DE:07EA`, `0C15:0137`, `08DE:07EF`, `08DE:0828`, `08DE:1816`,
`0F2D:0626`, `08DE:1858`, `08DE:082B`, and `3582:3387` stops.
`MEMDUMPBIN 0000 0000 A0000` records private RAM. For each EGA plane,
select Graphics Controller register 4 with `OUTP 3CE 04`, select read map
0..3 with `OUTP 3CF`, and run `MEMDUMPBIN A000 03C0 DC0`. Restore read
map 0 and selector 5 before resuming. Keep dumps and PNGs under ignored
`resources/`.

```sh
cmake -S . -B resources/build-slot-a-row7-twelve \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-row7-twelve --target \
  poolrad_slot_a_row7_twelve_cell_printer_tests poolrad_slot_a_printer_page_probe
ctest --test-dir resources/build-slot-a-row7-twelve \
  -R '^poolrad_slot_a_row7_twelve_cell_printer_tests$' --output-on-failure
python3 tools/replay/slot_a_row7_twelve_cell_printer_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --run resources/oracle-runs/slot-a-text-ui-after-col38-p9-009 \
  --probe resources/build-slot-a-row7-twelve/poolrad_slot_a_printer_page_probe
```

The audit hash-locks original and loaded call/body spans, caller frames,
source, font, captured pages, first-view persistence, and exact C output.
Probe records remain in the ignored run.
