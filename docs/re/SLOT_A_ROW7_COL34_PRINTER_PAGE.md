# Saved slot-A first view: row-seven column-34 printer call

After [PR #350](https://github.com/huntergdavis/poolrad-decomp/pull/350)'s
row-seven column-17 printer return, the next reached call that changes the
sampled EGA page is `RES_00BA:0875 → 08EF → 0968 → RES_0709:0626`.
It prints one cell at row **7**, column **34**, color **10**. The existing
bounded C printer-page model predicts the entire immediate 14,080-byte page.
This is parity for one reached call, not a complete static frame or a new
full-routine translation.

Before tracing, `deja "Poolrad slot A after row7 col17 twelve-cell printer
immediate page bd641160 first later page-changing writer before 3582:3387"`
returned no focused session. PR #350's page hash and the merged
[printer model](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md) were reused as leads;
this fresh native run independently measured the call, frame, and page.

## Original and loaded route

The ignored run is `resources/oracle-runs/slot-a-text-ui-after-row7-p9-010/`,
prepared with `tools/oracle/prepare_run.sh`. DOS v1.3 `START.EXE`, `GAME.OVR`,
and slot-A save SHA-256 values are respectively
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private heavy DOSBox-X binary SHA-256 is
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

| Expanded original image span | Bytes | SHA-256 | Meaning |
| --- | ---: | --- | --- |
| `[0x13CB,0x1418)` | 77 | `c0ea76675dc4a2d237422f1aeb7174f95e9e4d8af00f3e9ab6179f1b38fa3349` | caller path `RES_00BA:082B..0878` |
| `[0x1415,0x1418)` | 3 | `c7f558d3483fe565e125e715e980d1ea4b5a5364b42fd7d728ca95d0d3baff68` | near call `RES_00BA:0875 → 08EF` |
| `[0x148F,0x1513)` | 132 | `c4d1ffe64d255ff80f1f812284a6145df3b56815fdf4a4aadff777798e32cb0d` | bounded `RES_00BA:08EF` body through `RETF 8` |
| `[0x1508,0x150D)` | 5 | `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23` | far call `RES_00BA:0968 → RES_0709:0626` |
| `[0x76B6,0x7737)` | 129 | `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed` | bounded printer body |

The corresponding loaded spans `08DE:082B..0878`, `08DE:0875..0878`,
`08DE:08EF..0973`, `08DE:0968..096D`, and `0F2D:0626..06A7` hash to
`c0ea76675dc4a2d237422f1aeb7174f95e9e4d8af00f3e9ab6179f1b38fa3349`,
`c7f558d3483fe565e125e715e980d1ea4b5a5364b42fd7d728ca95d0d3baff68`,
`75134177259836ec8b3357d2735e0063d38d50a6045ab145e6f432719ecbccbd`,
`cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`,
and `8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`.
The loaded far call targets `0F2D:0626`.

At this invocation, the `08DE:082B` loop frame gives row 7, column 17,
and node pointer `44F5:0008`; that node's `+0x111` field is `0x3A`.
The next reached `08DE:0875` call enters `08EF` with return `08DE:0878`,
row 7, field selector `0x22`, and the same node pointer. Its nested printer
entry has `SS:SP=1D16:3C96`, return `08DE:096D`, source `1D16:3DA6`,
color 10, row 7, and column 34. The Pascal source length is **1**;
its length-plus-content SHA-256 is
`7f774bb46e7e342a2d9d0514b27cee622012f7415645345054f8cd31e1dda661`.
The live 1,416-byte font SHA-256 is
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
`DS:4A1E=1` selects EGA, and both page segments are `A000`.

## Same-run page contribution

Each page concatenates four 3,520-byte EGA planes from
`A000:[0x03C0,0x1180)`. The row-seven `08DE:082B` page equals PR #350's
immediate return. The `08DE:0875` and `08DE:0968` pre-call pages are also
byte-identical to it. These snapshots cannot exclude intervening writes
later restored.

| Stop | 14,080-byte page SHA-256 |
| --- | --- |
| `08DE:082B`, `08DE:0875`, `08DE:0968` | `bd641160cb016bb6ff8ec5043235c972ae829e910a589afc3618ea7b7ab34f6f` |
| `08DE:096D` printer return and `08DE:0878` parent return | `1dccb5ff277e6a49bbeb8392f4faa000ab7f71357caf87e9d7cc852b8eef2574` |
| `3582:3387` first-view return | `d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b` |

One cell × eight scanlines × four planes gives **32** modeled plane-byte
stores. The immediate snapshots differ at **14** positions: column 34 on
global scanlines 56–62 in planes 1 and 3. Planes 0 and 2 and scanline 63
have no net value change. All 14 values persist at first-view return.
Another **399** page bytes differ after the printer return; their writers
are unassigned. The total first-view difference from this call's entry is
**413** bytes. Snapshot deltas do not count same-value stores.

Given the run's ignored source, font, and entry page,
`pr_slot_a_printer_page_apply` reports one cell, 32 stores, and 14 changed
values. Its output matches **14,080/14,080** native immediate post-call page
bytes. The public synthetic test uses an invented glyph and page and checks
plane selection, row/column targeting, neighboring cells, repeat behavior,
and bounds. No game text, font bits, RAM, image, or page payload is tracked.

## Reproduce locally

`08DE:082B` is shared across rows 4–7 in this replay. Its first hit is
row 4, column 17, so the address alone is not an anchor. Select the hit
whose frame is row 7, column 17 and whose EGA page SHA-256 is
`bd641160cb016bb6ff8ec5043235c972ae829e910a589afc3618ea7b7ab34f6f`.
Then capture the next `08DE:0875`, `08DE:0968`, `0F2D:0626`,
`08DE:096D`, `08DE:0878`, and `3582:3387` stops. At each relevant stop,
`MEMDUMPBIN 0000 0000 A0000` records private RAM. For each EGA plane,
select Graphics Controller register 4 with `OUTP 3CE 04`, select read map
0..3 with `OUTP 3CF`, then run `MEMDUMPBIN A000 03C0 DC0`. Restore read
map 0 and selector 5 before resuming. Keep all dumps under ignored
`resources/`.

```sh
cmake -S . -B resources/build-slot-a-row7-col34 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-row7-col34 --target \
  poolrad_slot_a_row7_col34_printer_tests poolrad_slot_a_printer_page_probe
ctest --test-dir resources/build-slot-a-row7-col34 \
  -R '^poolrad_slot_a_row7_col34_printer_tests$' --output-on-failure
python3 tools/replay/slot_a_row7_col34_printer_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --run resources/oracle-runs/slot-a-text-ui-after-row7-p9-010 \
  --probe resources/build-slot-a-row7-col34/poolrad_slot_a_printer_page_probe
```

The audit hash-locks original and loaded call/body spans, caller frames,
source, font, every captured page plane, first-view persistence, and exact
C output. Probe records remain in the ignored run.
