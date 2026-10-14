# Saved slot-A first view: row-nine column-38 printer call

After [PR #365](https://github.com/huntergdavis/poolrad-decomp/pull/365)'s
row-nine column-34 return, the next reached page-changing printer call is
`RES_00BA:08B7 → 0973 → 09BB → 1037 → 09BE → RES_0709:0626`.
It prints **one** cell at row **9**, column **38**, color **10**, and
returns to `08DE:09C3`. The existing bounded C printer-page model
predicts its entire immediate 14,080-byte page. This is parity for one
reached call, not a complete rendered view or a new full-routine
translation.

Before tracing, `deja "Poolrad slot A row9 col38 printer after col34
remaining 14 first view page writer"` found no matching session. The
merged [column-34 page](SLOT_A_ROW9_COL34_PRINTER_PAGE.md),
[row-eight column-38 route](SLOT_A_ROW8_COL38_PRINTER_PAGE.md), and
[printer model](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md) were leads; this run
independently checks the live frame, original and loaded route, and page.

## Original and loaded route

The ignored run is
`resources/oracle-runs/slot-a-text-ui-after-row9-col34-p9-017/`, prepared
with `tools/oracle/prepare_run.sh`. DOS v1.3 `START.EXE`, `GAME.OVR`, and
slot-A save SHA-256 values are respectively
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private heavy DOSBox-X binary SHA-256 is
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

| Expanded original image span | Bytes | SHA-256 | Meaning |
| --- | ---: | --- | --- |
| `[0x1440,0x145A)` | 26 | `056fc9fc372c8e544e5fe890d6b728b7859e9444fee14e204127079a1e137ec1` | caller context `RES_00BA:08A0..08BA` |
| `[0x1457,0x145A)` | 3 | `e72bebba602f7eb4edd3783632f297e7b0bf432aa0b0438a34908168cce490a4` | near call `08B7 → 0973` |
| `[0x1513,0x1569)` | 86 | `a5093db76117998bf62e1d6a3b0ca1816f6109dd7d3bca570070a09c0f73ebd0` | bounded `0973` body through `RETF 0x0A` |
| `[0x155B,0x155E)` | 3 | `0924e790e7d93bf418508fcbf8f4043c33b243a114d0dd47f665962626764f53` | near call `09BB → 1037` formatter boundary |
| `[0x155E,0x1563)` | 5 | `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23` | far call `09BE → RES_0709:0626` |
| `[0x76B6,0x7737)` | 129 | `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed` | bounded printer body |

The corresponding loaded spans `08DE:08A0..08BA`, `08DE:08B7..08BA`,
`08DE:0973..09C9`, `08DE:09BB..09BE`, `08DE:09BE..09C3`, and
`0F2D:0626..06A7` hash to
`056fc9fc372c8e544e5fe890d6b728b7859e9444fee14e204127079a1e137ec1`,
`e72bebba602f7eb4edd3783632f297e7b0bf432aa0b0438a34908168cce490a4`,
`c7577505151147a243592d02b42335233680a27273dc5df3f427d4e778a3c3d3`,
`0924e790e7d93bf418508fcbf8f4043c33b243a114d0dd47f665962626764f53`,
`cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`,
and `8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`.
The original far call targets `0709:0626`; its loaded target is
`0F2D:0626`.

At the previous `08DE:096D` return, the parent frame passes row 9,
column 34, and source `4529:0008`. The next printer entry frame at
`SS:3D94` gives return `08DE:09C3`, private source `1D16:3DA2`,
color 10, row 9, and column 38. The `0973` parent frame at `SS:3EA4`
returns to `08DE:08BA`, passes row 9 and column 38, node pointer
`4529:0008`, and zero override. Live node bytes `+0x11B` and `+0x32`
both equal **6**; the equal branch selects color 10. The Pascal source
length is **1**; its length-plus-content SHA-256 is
`17333b15b4a5afd16cac55a104b554fc63cc8731c76249dddce5ec3363fe4f38`.
The live 1,416-byte font SHA-256 is
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
`DS:4A1E=1` selects EGA, and both page segments are `A000`.
The formatter's complete text effects remain outside this page projection.

## Same-run page contribution

Each page concatenates four 3,520-byte EGA planes from
`A000:[0x03C0,0x1180)`. The prior `08DE:096D` page and this
`0F2D:0626` entry page are byte-identical. These snapshots cannot exclude
intervening writes later restored.

| Stop | 14,080-byte page SHA-256 |
| --- | --- |
| `08DE:096D` and next `0F2D:0626` entry | `4abd88a43973afe33f43c772d6e7988cf83da3106159d802490f14db73dd32d6` |
| `08DE:09C3` immediate printer return | `d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b` |
| `3582:3387` first-view return | `d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b` |

One cell × eight scanlines × four planes gives **32** modeled plane-byte
stores. Immediate snapshots differ at **14** positions: column 38 on
global scanlines 72–78 in planes 1 and 3. Planes 0 and 2 and scanline
79 have no net value change. All 14 values persist at first-view return;
the immediate and first-view captured pages are identical over **14,080
bytes**. This leaves **zero later net differences in this page window**.
Snapshot deltas do not count same-value or transient writes and do not
establish a complete rendered frame.

Given this run's ignored source, font, and entry page,
`pr_slot_a_printer_page_apply` reports one cell, 32 stores, and 14 changed
values. Its output matches **14,080/14,080** native immediate
post-call page bytes. The public synthetic test checks row-nine placement,
plane selection, and neighboring rows and columns using invented data.
No game text, font bits, RAM, image, or page payload is tracked.

## Reproduce locally

Prepare a fresh ignored run and start the pinned heavy DOSBox-X with
`DEBUGBOX START`. At the title press Return; at the main menu press `l`;
at `LOAD WHICH GAME` press `a`. Arm `BP 08DE:09C3`; select the fifth hit
whose parent frame gives row 8 and column 38. Then arm
`BP 08DE:096D` for the row-nine column-34 return with page hash
`4abd88a43973afe33f43c772d6e7988cf83da3106159d802490f14db73dd32d6`.
Clear that breakpoint and arm `BP 0F2D:0626` for the next printer entry.
Arm `BP 08DE:09C3` for its immediate return, then `BP 3582:3387`
for first-view return. At each stop, `MEMDUMPBIN 0000 0000 A0000`
records private RAM. For each EGA plane, select Graphics Controller
register 4 with `OUTP 3CE 04`, select read map 0..3 with `OUTP 3CF`,
and run `MEMDUMPBIN A000 03C0 DC0`. Restore read map 0 and selector 5
before resuming. Keep all dumps under ignored `resources/`.

```sh
cmake -S . -B resources/build-slot-a-row9-col38 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-row9-col38 --target \
  poolrad_slot_a_row9_col38_printer_tests poolrad_slot_a_printer_page_probe
ctest --test-dir resources/build-slot-a-row9-col38 \
  -R '^poolrad_slot_a_row9_col38_printer_tests$' --output-on-failure
python3 tools/replay/slot_a_row9_col38_printer_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --run resources/oracle-runs/slot-a-text-ui-after-row9-col34-p9-017 \
  --probe resources/build-slot-a-row9-col38/poolrad_slot_a_printer_page_probe
```

The audit hash-locks original and loaded call/body spans, caller frames,
branch inputs, source, font, every captured page plane, first-view
persistence, and exact C output. Probe records remain in the ignored run.
