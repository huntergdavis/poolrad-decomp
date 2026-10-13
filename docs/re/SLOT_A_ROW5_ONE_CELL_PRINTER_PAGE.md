# Saved slot-A first view: row-five one-cell printer invocation

The saved slot-A route reaches another invocation of the previously
recovered `RES_00BA:0968` far call to `RES_0709:0626`, after the six-cell
call returning at `RES_00BA:1858`. This invocation prints one cell at
row **5**, column **34**. The merged
[earlier audit](SLOT_A_NEXT_CLEAN_PRINTER_PAGE.md) measured the same
call site at row **4**, column **34**. The row-five invocation changes
**14** more EGA plane bytes. Of the **868** changed page values after
the six-cell return, **854** remain unattributed. The existing bounded
C printer-page model matches this invocation's immediate 14,080-byte
output. This work makes no full first-view or full-C routine claim.

Before replay, `deja "Poolrad saved slot A after RES_00BA 1858 six-cell
printer next UI glyph 868 page changes"` had no focused session.
The merged first and second printer notes and the separately measured
[six-cell call in PR #334](https://github.com/huntergdavis/poolrad-decomp/pull/334)
provided route leads. The following caller frame and three page stops
were measured afresh in a new ignored replay.

## Original and live call

The run is `resources/oracle-runs/slot-a-text-ui-post-1858-p9-004/`,
prepared with `tools/oracle/prepare_run.sh`. Its ignored DOS v1.3
`START.EXE`, `GAME.OVR`, and slot-A save SHA-256 values are
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private heavy DOSBox-X executable hashes to
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

The original five-byte far call `RES_00BA:0968..096C` is expanded
image `[0x01508,0x0150D)`, SHA-256
`04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23`;
its numeric target is `RES_0709:0626`. The relocated live call at
`08DE:0968..096C` hashes to
`cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`.
The original printer body `[0x076B6,0x07737)` (129 bytes) hashes to
`b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`;
the loaded `0F2D:0626..06A6` span hashes to
`8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`.

At the newly reached printer entry `0F2D:0626`, `SS:SP=1D16:3C96`
and the far return is `08DE:096D`. The source pointer is
`1D16:3DA6`; its Pascal length is **1** and its length-plus-content
SHA-256 is `9057f89eb79a4aa2452527d567cb057bbdcd4d23c5e6cc2039bc3bc61f1230c6`.
The remaining argument words are color **10**, row **5**, column
**34**. The live 1,416-byte font buffer SHA-256 is
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
`DS:4A1E=1` selects EGA and both page segment words are `A000`.
The call returned at `08DE:096D`; this same replay then reached
first-view return `3582:3387`.

## Same-run EGA page contribution

Each page snapshot concatenates four 3,520-byte EGA planes from
`A000:[0x03C0,0x1180)`. The entry page hash equals the six-cell
call's post-page hash measured for PR #334 in a separate replay. The
first-view page hash matches the previously measured first-view page.
Boundary equality does not rule out intervening writes and restoration.

| Stop | 640 KiB RAM SHA-256 | 14,080-byte page SHA-256 |
| --- | --- | --- |
| row-five printer entry | `bfcf562d4f20c22f684a7862121fee9d785ddd1bdfa4fd8096ccaa6d14133b11` | `b48537bb34935271cef3797cd3eff166d70eebcec2e08db3fd0c6300a511a128` |
| immediate printer return | `bf0084b78468585522761302efaeef3406a513fd3ca6c47bbe3100ae2c4ad766` | `721b9e030448fa2c363585f0726d469a2a4cdd61d61d2fecae6195480219dfe0` |
| first-view return | `25a38fd9014ef8368ca377203a59035768f9bfce242b83a2f5aa120dec0b139d` | `d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b` |

The cell occupies column **34**, scanlines **40–47**. The modeled
eight rows times four planes imply **32 plane-byte stores**. Native
pre/post snapshots differ at **14** addresses: **7** each in planes
1 and 3, on scanlines 40–46. All 14 post-call values are present at
first-view return. The later snapshot has **854** changed plane bytes
outside this cell and no net change inside it. Thus the measured
page remainder from this entry is **14 + 854 = 868** changed values.
Snapshot deltas do not measure same-value stores.

With this run's private hash-checked source, font, and pre-page,
`pr_slot_a_printer_page_apply` reports one cell, 32 modeled stores,
and 14 value changes. It predicts **14,080/14,080** native immediate
post-call page bytes. The checker pins the original and loaded code,
caller frame, source and font, every plane at each stop, the later
remainder, and C output. A synthetic test uses an invented glyph to
check row-five placement, planes 1/3, unchanged neighbors and prior
row, repeat-call idempotence, and a bounds rejection without mutation.
No game text, font bits, RAM, image, or page payload is tracked.

## Reproduce locally

Prepare a fresh ignored run. In the heavy debugger, arm
`BP 08DE:1858` before selecting slot A. At that return, arm
`BP 0F2D:0626` and `BP 3582:3387`, then run; the printer entry is
reached first. Arm `BP 08DE:096D` for the immediate return. Clear
intermediate breakpoints with `BPDEL *` and arm `BP 3582:3387` to
capture the first-view boundary. At each stop, use
`MEMDUMPBIN 0000 0000 A0000` for RAM. For each EGA plane, select
read-map register 4 with `OUTP 3CE 04`, select plane 0..3 with
`OUTP 3CF`, then `MEMDUMPBIN A000 03C0 DC0`; restore read map 0
and selector 5 before resuming. Keep all captures under ignored
`resources/`.

```sh
cmake -S . -B resources/build-slot-a-row5-one \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-row5-one --target \
  poolrad_slot_a_row5_one_cell_printer_tests poolrad_slot_a_printer_page_probe
ctest --test-dir resources/build-slot-a-row5-one \
  -R '^poolrad_slot_a_row5_one_cell_printer_tests$' --output-on-failure
python3 tools/replay/slot_a_row5_one_cell_printer_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --run resources/oracle-runs/slot-a-text-ui-post-1858-p9-004 \
  --probe resources/build-slot-a-row5-one/poolrad_slot_a_printer_page_probe
```

The audit's generated probe records stay in the ignored run.
