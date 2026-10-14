# Saved slot-A first view: row-seven column-37 two-cell printer call

After [PR #352](https://github.com/huntergdavis/poolrad-decomp/pull/352)'s
row-seven column-34 printer return, the next reached call that changes the
sampled EGA page is `RES_00BA:08B7 → 0973 → 09BE → RES_0709:0626`.
It prints **two** cells at row **7**, columns **37–38**, color **10**.
The existing bounded C printer-page model predicts the entire immediate
14,080-byte page. This is parity for one reached call, not a complete
static frame or a new full-routine translation.

Before tracing, `deja "Poolrad saved slot A after row7 col34 one cell printer
immediate EGA page 1dccb5ff first later page changing writer before 3582:3387"`
was invoked. A shorter retry, `deja "Poolrad slot A row7 col34 next UI writer"`,
timed out after 15 seconds without a focused result. The merged
[PR #350 row-seven page](SLOT_A_ROW7_TWELVE_CELL_PRINTER_PAGE.md),
[PR #352 column-34 page](SLOT_A_ROW7_COL34_PRINTER_PAGE.md), and
[printer model](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md) were reused as page and
C-model leads. This fresh native run independently measured the call,
frame, and immediate page.

## Original and loaded route

The ignored run is
`resources/oracle-runs/slot-a-text-ui-after-col34-row7-p9-011/`, prepared
with `tools/oracle/prepare_run.sh`. DOS v1.3 `START.EXE`, `GAME.OVR`, and
slot-A save SHA-256 values are respectively
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private heavy DOSBox-X binary SHA-256 is
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

| Expanded original image span | Bytes | SHA-256 | Meaning |
| --- | ---: | --- | --- |
| `[0x1418,0x145A)` | 66 | `a0912afb2db94228642cb606dc750911d43961fc928e5549d0c19c8a3c3ae95a` | caller path `RES_00BA:0878..08BA` |
| `[0x1457,0x145A)` | 3 | `e72bebba602f7eb4edd3783632f297e7b0bf432aa0b0438a34908168cce490a4` | near call `RES_00BA:08B7 → 0973` |
| `[0x1513,0x1569)` | 86 | `a5093db76117998bf62e1d6a3b0ca1816f6109dd7d3bca570070a09c0f73ebd0` | bounded `RES_00BA:0973` body through `RETF 0x0A` |
| `[0x155E,0x1563)` | 5 | `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23` | far call `RES_00BA:09BE → RES_0709:0626` |
| `[0x76B6,0x7737)` | 129 | `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed` | bounded printer body |

The corresponding loaded spans `08DE:0878..08BA`, `08DE:08B7..08BA`,
`08DE:0973..09C9`, `08DE:09BE..09C3`, and `0F2D:0626..06A7` hash to
`a0912afb2db94228642cb606dc750911d43961fc928e5549d0c19c8a3c3ae95a`,
`e72bebba602f7eb4edd3783632f297e7b0bf432aa0b0438a34908168cce490a4`,
`c7577505151147a243592d02b42335233680a27273dc5df3f427d4e778a3c3d3`,
`cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`,
and `8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`.
The loaded far call targets `0F2D:0626`.

At this invocation, the `08DE:0878` loop frame gives row 7, column 17,
and node pointer `44F5:0008`; the node's `+0x11B` byte is `0x0A`.
The `08DE:08B7` call enters `0973` with return `08DE:08BA`, row 7,
selector `0x25`, and the same node pointer. Its nested printer entry has
`SS:SP=1D16:3D94`, return `08DE:09C3`, source `1D16:3DA2`, color 10,
row 7, and column 37. The Pascal source length is **2**, and its
length-plus-content SHA-256 is
`69f57e18a3f92716d6ead17fcc66a626dddade0965bbde25f5f7accebd719a30`.
The live 1,416-byte font SHA-256 is
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
`DS:4A1E=1` selects EGA, and both page segments are `A000`.

## Same-run page contribution

Each page concatenates four 3,520-byte EGA planes from
`A000:[0x03C0,0x1180)`. The row-seven `08DE:0878` page equals PR #352's
immediate return. Pages at `08DE:08B7` and `08DE:09BE` are byte-identical
to it. These snapshots cannot exclude intervening writes later restored.

| Stop | 14,080-byte page SHA-256 |
| --- | --- |
| `08DE:0878`, `08DE:08B7`, `08DE:09BE` | `1dccb5ff277e6a49bbeb8392f4faa000ab7f71357caf87e9d7cc852b8eef2574` |
| `08DE:09C3` printer return and `08DE:08BA` parent return | `99ce3c3496cf59d0f0e43b67ac66d03ad0e09af30168d19ea0bf55877500cbc0` |
| `3582:3387` first-view return | `d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b` |

Two cells × eight scanlines × four planes gives **64** modeled plane-byte
stores. Immediate snapshots differ at **28** positions: columns 37–38 on
global scanlines 56–62 in planes 1 and 3. Planes 0 and 2 and scanline 63
have no net value change. All 28 values persist at first-view return.
Another **371** page bytes differ after the printer return; their writers
are unassigned. The total first-view difference from this call's entry is
**399** bytes. Snapshot deltas do not count same-value stores.

Given this run's ignored source, font, and entry page,
`pr_slot_a_printer_page_apply` reports two cells, 64 stores, and 28 changed
values. Its output matches **14,080/14,080** native immediate post-call
page bytes. The public synthetic test uses two distinct invented glyphs
and an invented page to check adjacent placement, plane selection,
neighbors, repeat behavior, and bounds. No game text, font bits, RAM,
image, or page payload is tracked.

## Reproduce locally

`08DE:0878` is shared across rows 4–7 in this replay. Its first hit is
row 4, column 17, so the address alone is not an anchor. Select the hit
whose frame is row 7, column 17 and whose EGA page SHA-256 is
`1dccb5ff277e6a49bbeb8392f4faa000ab7f71357caf87e9d7cc852b8eef2574`.
Then capture the next `08DE:08B7`, `08DE:0973`, `08DE:09BE`,
`0F2D:0626`, `08DE:09C3`, `08DE:08BA`, and `3582:3387` stops.
`MEMDUMPBIN 0000 0000 A0000` records private RAM. For each EGA plane,
select Graphics Controller register 4 with `OUTP 3CE 04`, select read map
0..3 with `OUTP 3CF`, and run `MEMDUMPBIN A000 03C0 DC0`. Restore read
map 0 and selector 5 before resuming. Keep all dumps under ignored
`resources/`.

```sh
cmake -S . -B resources/build-slot-a-row7-col37 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-row7-col37 --target \
  poolrad_slot_a_row7_col37_two_cell_printer_tests poolrad_slot_a_printer_page_probe
ctest --test-dir resources/build-slot-a-row7-col37 \
  -R '^poolrad_slot_a_row7_col37_two_cell_printer_tests$' --output-on-failure
python3 tools/replay/slot_a_row7_col37_two_cell_printer_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --run resources/oracle-runs/slot-a-text-ui-after-col34-row7-p9-011 \
  --probe resources/build-slot-a-row7-col37/poolrad_slot_a_printer_page_probe
```

The audit hash-locks original and loaded call/body spans, caller frames,
source, font, every captured page plane, first-view persistence, and
exact C output. Probe records remain in the ignored run.
