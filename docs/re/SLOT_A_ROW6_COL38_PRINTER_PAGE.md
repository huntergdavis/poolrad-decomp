# Saved slot-A first view: row-six column-38 printer call

After [PR #344](https://github.com/huntergdavis/poolrad-decomp/pull/344)'s
row-six column-34 printer return, the next reached page-changing call is
another `RES_0709:0626` printer invocation. Its entry page equals PR #344's
return page. This call prints **one** cell at row **6**, column **38**, changing
**14** EGA plane bytes. The existing bounded C printer-page model predicts
the entire immediate 14,080-byte page. This is parity for one reached call,
not a complete static frame or a new full-routine translation.

Before tracing, `deja "Poolrad slot A after RES_00BA 0968 row6 column34 printer
next page-changing call 658 bytes"` was invoked. Local indexing stalled, so
it supplied no focused session result. PR #344's page hash and the merged
[printer model](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md) were reused as route and
C-model leads; this fresh run measured the next call independently.

## Original and loaded call

The ignored run is `resources/oracle-runs/slot-a-text-ui-after-col34-p9-008/`,
prepared with `tools/oracle/prepare_run.sh`. DOS v1.3 `START.EXE`,
`GAME.OVR`, and slot-A save SHA-256 values are respectively
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private heavy DOSBox-X binary SHA-256 is
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

| Expanded original image span | Length | SHA-256 | Meaning |
| --- | ---: | --- | --- |
| `[0x155E,0x1563)` | 5 | `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23` | `RES_00BA:09BE` far call to `RES_0709:0626`, return `09C3` |
| `[0x76B6,0x7737)` | 129 | `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed` | bounded printer body |

The loaded call at `08DE:09BE` has SHA-256
`cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`
and targets `0F2D:0626`. The loaded printer body at
`0F2D:0626..06A6` has SHA-256
`8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`.
At printer entry, `SS:SP=1D16:3D94`; the stack words give return
`08DE:09C3`, source `1D16:3DA2`, color **10**, row **6**, and column
**38**. The Pascal source length is **1**, and its length-plus-content SHA-256
is `06f647b157b8557a12979ba04cf5ba222b9747cf5bbe82f24a48e25c0c508042`.
The live 1,416-byte font SHA-256 is
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
`DS:4A1E=1` selects EGA and both page segments are `A000`.

## Same-run page contribution

Each page concatenates four 3,520-byte EGA planes from
`A000:[0x03C0,0x1180)`. The anchored `08DE:096D` baseline is the exact
post-PR #344 page. The next printer entry is byte-identical to that
baseline; snapshots cannot exclude intervening writes later restored.

| Stop | 14,080-byte page SHA-256 |
| --- | --- |
| `08DE:096D` baseline | `29a78a982037738bc707b584563799657f3ab8ba15981552438e9452e8650512` |
| `0F2D:0626` entry | `29a78a982037738bc707b584563799657f3ab8ba15981552438e9452e8650512` |
| `08DE:09C3` immediate return | `a6419841fb5aa3e5a40a401c081b69be718b692af57dd3a3d29b58e03ed8158d` |
| `3582:3387` first-view return | `d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b` |

One cell × eight scanlines × four planes gives **32** modeled plane-byte
stores. Immediate snapshots differ at **14** positions: column 38 on global
scanlines 48–54 in planes 1 and 3. Plane 0, plane 2, and scanline 55 have
no net value change. All 14 values persist at first-view return. Another
**644** page bytes differ after this return; their writers are unassigned.
The total first-view difference from this call's entry is **658** bytes.
Snapshot deltas do not count same-value stores.

Given this run's ignored source, font, and entry page,
`pr_slot_a_printer_page_apply` reports one cell, 32 stores, and 14 changed
values. Its output matches **14,080/14,080** native immediate post-call
page bytes. The public synthetic test uses an invented glyph and page and
checks row/column targeting, neighboring cells, repeat behavior, and bounds.
The ignored paired native/C visual covers **320×88** pixels at global
scanlines 24–111, with **28,160/28,160** pixel indices equal; its diff has
zero mismatches. These are diagnostic renderings of the measured page, not
a claim of full C static-frame parity. The paired and diff PNG SHA-256 values
are `f5cad7eee3d677e8e1b372680411fa08b13f64015a548ca36e0f60987b18f95f`
and `3bd8ab0865a4bafcbf9707334c5d8ac68f5e510f51d43c988cf6263cc9d2f030`.

## Reproduce locally

The saved route calls the same printer functions more than once. An earlier
`08DE:096D` return belongs to row 4, and another to row 5. Anchor the
target by first reaching the row-five `08DE:09C3` page with SHA-256
`304a1a91d14d0e80bcb5c97053ab6618b6b8a7ab5d463977521e8c448968dd86`.
The next `0F2D:0626 → 08DE:1858` prints PR #340's eight cells;
the following `0F2D:0626 → 08DE:096D` prints PR #344's column-34 cell.
Capture that baseline, then the next `0F2D:0626` entry, its
`08DE:09C3` return, and first-view return `3582:3387`.
`MEMDUMPBIN 0000 0000 A0000` records private RAM. For each EGA plane,
select Graphics Controller register 4 with `OUTP 3CE 04`, select read map
0..3 with `OUTP 3CF`, and run `MEMDUMPBIN A000 03C0 DC0`. Restore read
map 0 and selector 5 before resuming. Keep all dumps and PNGs under
ignored `resources/`.

```sh
cmake -S . -B resources/build-slot-a-row6-col38 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-row6-col38 --target \
  poolrad_slot_a_row6_col38_printer_tests poolrad_slot_a_printer_page_probe
ctest --test-dir resources/build-slot-a-row6-col38 \
  -R '^poolrad_slot_a_row6_col38_printer_tests$' --output-on-failure
python3 tools/replay/slot_a_row6_col38_printer_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --run resources/oracle-runs/slot-a-text-ui-after-col34-p9-008 \
  --probe resources/build-slot-a-row6-col38/poolrad_slot_a_printer_page_probe
```

The audit hash-locks the original and loaded call/body, caller frame,
source, font, captured pages, first-view persistence, and exact C output.
Probe records remain in the ignored run.
