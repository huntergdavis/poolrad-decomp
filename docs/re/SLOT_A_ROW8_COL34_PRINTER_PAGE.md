# Saved slot-A first view: row-eight column-34 printer call

After [PR #356](https://github.com/huntergdavis/poolrad-decomp/pull/356)'s
row-eight eight-cell return, the next reached page-changing call is
`RES_00BA:0875 → 08EF → 0968 → RES_0709:0626`. It prints **one** cell
at row **8**, column **34**, color **10**, and returns to `08DE:096D`.
The existing bounded C printer-page model predicts the entire immediate
14,080-byte page. This is parity for one reached call, not a complete static
frame or a new full-routine translation.

Before tracing, the broad `deja` query for the next row-eight printer returned
no focused session; `deja "row8 printer"` timed out after 15 seconds.
The merged [PR #356 page](SLOT_A_ROW8_EIGHT_CELL_PRINTER_PAGE.md) and
[printer model](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md) were leads. This run
independently checks the live path, frame, and immediate page.

## Original and loaded route

The ignored run is
`resources/oracle-runs/slot-a-text-ui-after-row8-eight-p9-013/`, prepared
with `tools/oracle/prepare_run.sh`. DOS v1.3 `START.EXE`, `GAME.OVR`, and
slot-A save SHA-256 values are respectively
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private heavy DOSBox-X binary SHA-256 is
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

| Expanded original image span | Bytes | SHA-256 | Meaning |
| --- | ---: | --- | --- |
| `[0x1405,0x1418)` | 19 | `032f435749f75440b10833d3dc4d575882c2604972ed380b837e3f6ea15cc553` | caller context `RES_00BA:0865..0878` |
| `[0x1415,0x1418)` | 3 | `c7f558d3483fe565e125e715e980d1ea4b5a5364b42fd7d728ca95d0d3baff68` | near call `RES_00BA:0875 → 08EF` |
| `[0x148F,0x1513)` | 132 | `c4d1ffe64d255ff80f1f812284a6145df3b56815fdf4a4aadff777798e32cb0d` | bounded `RES_00BA:08EF` body through `RETF 0x08` |
| `[0x1508,0x150D)` | 5 | `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23` | far call `RES_00BA:0968 → RES_0709:0626` |
| `[0x76B6,0x7737)` | 129 | `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed` | bounded printer body |

The corresponding loaded spans `08DE:0865..0878`, `08DE:0875..0878`,
`08DE:08EF..0973`, `08DE:0968..096D`, and `0F2D:0626..06A7` hash to
`032f435749f75440b10833d3dc4d575882c2604972ed380b837e3f6ea15cc553`,
`c7f558d3483fe565e125e715e980d1ea4b5a5364b42fd7d728ca95d0d3baff68`,
`75134177259836ec8b3357d2735e0063d38d50a6045ab145e6f432719ecbccbd`,
`cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`,
and `8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`.
The original far call targets `0709:0626`; its loaded target is `0F2D:0626`.

At the next printer entry `0F2D:0626`, `SS:SP=1D16:3C96`.
Its frame gives return `08DE:096D`, private source `1D16:3DA6`, color 10,
row 8, and column 34. The `08EF` parent frame returns to `08DE:0878`
and passes row 8, column 34, and node pointer `4513:0008`.
The Pascal source length is **1**; its length-plus-content SHA-256 is
`06f647b157b8557a12979ba04cf5ba222b9747cf5bbe82f24a48e25c0c508042`.
The live 1,416-byte font SHA-256 is
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
`DS:4A1E=1` selects EGA, and both page segments are `A000`.

## Same-run page contribution

Each page concatenates four 3,520-byte EGA planes from
`A000:[0x03C0,0x1180)`. The prior `08DE:1858` page and this
`0F2D:0626` entry page are byte-identical. These snapshots cannot exclude
intervening writes later restored.

| Stop | 14,080-byte page SHA-256 |
| --- | --- |
| `08DE:1858` and next `0F2D:0626` entry | `8723a10b9ac3747fd9edc42003a59c2434d1aa734fab75db17483eebd0b95498` |
| `08DE:096D` immediate printer return | `f338875f479952bce7d45467ad8aed9d35bae76ef2a6f1dafe0b7a78273c09b9` |
| `3582:3387` first-view return | `d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b` |

One cell × eight scanlines × four planes gives **32** modeled plane-byte
stores. Immediate snapshots differ at **14** positions: column 34 on global
scanlines 64–70 in planes 1 and 3. Planes 0 and 2 and scanline 71 have no
net value change. All 14 values persist at first-view return. Another
**189** page bytes differ after this return; their writers are unassigned.
The total first-view difference from this call's entry is **203** bytes.
Snapshot deltas do not count same-value stores.

Given this run's ignored source, font, and entry page,
`pr_slot_a_printer_page_apply` reports one cell, 32 stores, and 14 changed
values. Its output matches **14,080/14,080** native immediate post-call
page bytes. The public synthetic test uses an invented glyph and page to
check placement, plane selection, neighbors, repeat behavior, and bounds.
No game text, font bits, RAM, image, or page payload is tracked.

## Reproduce locally

Start the pinned heavy DOSBox-X with the prepared ignored run. At the
code-word prompt press Return; at the main menu press `l`; at `LOAD WHICH
GAME` press `a`. Use visible screen gates for those keys. Arm
`BP 08DE:08BA`; its first four hits have row bytes 4, 5, 6, and 7.
Then arm `BP 08DE:1858` and capture the row-eight eight-cell return with
page SHA-256 `8723a10b9ac3747fd9edc42003a59c2434d1aa734fab75db17483eebd0b95498`.
Clear that breakpoint and arm `BP 0F2D:0626` for the next printer entry.
Arm `BP 08DE:096D` for its immediate return, then `BP 3582:3387` for
first-view return. At each stop, `MEMDUMPBIN 0000 0000 A0000` records
private RAM. For each EGA plane, select Graphics Controller register 4
with `OUTP 3CE 04`, select read map 0..3 with `OUTP 3CF`, and run
`MEMDUMPBIN A000 03C0 DC0`. Restore read map 0 and selector 5 before
resuming. Keep all dumps under ignored `resources/`.

```sh
cmake -S . -B resources/build-slot-a-row8-col34 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-row8-col34 --target \
  poolrad_slot_a_row8_col34_printer_tests poolrad_slot_a_printer_page_probe
ctest --test-dir resources/build-slot-a-row8-col34 \
  -R '^poolrad_slot_a_row8_col34_printer_tests$' --output-on-failure
python3 tools/replay/slot_a_row8_col34_printer_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --run resources/oracle-runs/slot-a-text-ui-after-row8-eight-p9-013 \
  --probe resources/build-slot-a-row8-col34/poolrad_slot_a_printer_page_probe
```

The audit hash-locks original and loaded call/body spans, caller frames,
source, font, every captured page plane, first-view persistence, and exact
C output. Probe records remain in the ignored run.
