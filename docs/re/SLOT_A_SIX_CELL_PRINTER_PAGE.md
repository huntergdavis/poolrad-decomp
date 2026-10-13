# Saved slot-A first view: next six-cell printer call

The live saved slot-A route reaches another `RES_0709:0626` EGA printer
call after the third printer return at `RES_00BA:09C3`. Its caller is
`RES_00BA:1853`, returning to `RES_00BA:1858`. This call accounts for
**126** of the **994** changed plane bytes between the third printer's
return and the first-view return; **868** remain unattributed. The
existing bounded C page model matches this call's immediate 14,080-byte
page output. This does not translate the full first view or the complete
resident printer, and adds no full-C routine or identity count.

`deja "Poolrad saved slot A first view after RES_00BA 09C3 next printer
TEXT UI EGA page delta"` had no focused match. A shorter `deja "slot A
printer"` found the earlier local heavy-debugger setup session
`01a12b3e-db2`. The merged [first printer audit](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md),
[second printer audit](SLOT_A_NEXT_CLEAN_PRINTER_PAGE.md), and
[font recovery](FONT_8X8D_201.md) supplied the replay method and ABI
lead. The intervening third-call boundary was measured for
[PR #332](https://github.com/huntergdavis/poolrad-decomp/pull/332).
The call and three page stops below were measured independently in a
new ignored replay.

## Original call and live frame

The ignored run is `resources/oracle-runs/slot-a-text-ui-after-09c3-p9-003/`.
Its DOS v1.3 `START.EXE`, `GAME.OVR`, and slot-A save SHA-256 values are
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private heavy DOSBox-X executable hashes to
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

The original five-byte far call at `RES_00BA:1853..1857` is expanded
image `[0x023F3,0x023F8)`, SHA-256
`04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23`.
Its numeric target is `RES_0709:0626`. The relocated live call at
`08DE:1853..1857` hashes to
`cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`.
The original printer body is `[0x076B6,0x07737)` (129 bytes), SHA-256
`b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`;
the relocated `0F2D:0626..06A6` span hashes to
`8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`.

At `0F2D:0626`, `SS:SP=1D16:3E92`; the far return is `08DE:1858`.
The source pointer `44BB:0008` holds a Pascal length of **6** and its
length-plus-content hashes to
`e181017de583f193d09392267f0c03e789225c9bbe5265095326e792d5f3b6c5`.
The other argument words are color **11**, row **5**, column **17**.
The live 1,416-byte font buffer hashes to
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
`DS:4A1E=1` selects EGA and both page segment words are `A000`.
The call returned to `08DE:1858`; the same run reached first-view
return `3582:3387`.

## Same-run EGA page effect

Each page is four 3,520-byte planes from `A000:[0x03C0,0x1180)`,
concatenated in plane order. The call-entry page is exactly the
third-call return page measured for PR #332. The first-view page hash
also matches the earlier first-view captures. These boundary equalities
do not rule out intervening same-value writes or restoration.

| Stop | 640 KiB RAM SHA-256 | 14,080-byte page SHA-256 |
| --- | --- | --- |
| six-cell printer entry | `3e3ed4dd8f27ff6d338879260f7f6128069313bd3c7ac7bc8ceaf8fed71bb153` | `00a5a7693604b8f8487ced7ab081c04a3ba5dffd2b8f0b749c9ff80919267d96` |
| immediate printer return | `9be41c240e4896c3f98cf64bc6eab66ca2e1eeee9fb9bfed5dfe45b3710459f6` | `b48537bb34935271cef3797cd3eff166d70eebcec2e08db3fd0c6300a511a128` |
| first-view return | `f67e100ee3d16222bfa49beb5bd05c02529a2d740e2918b41e5f0b449e8fe660` | `d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b` |

The six cells occupy columns **17–22**, scanlines **40–47**. The
printer's eight rows times six cells times four planes imply **192
modeled plane-byte stores**. Native pre/post snapshots differ at
**126** addresses: **42 each** in planes 0, 1, and 3, all on scanlines
40–46 within those columns. All 126 post-call values are present at
first-view return. The later snapshot has **868** changed bytes outside
these six cells and no net change inside them. Thus the measured page
remainder is **126 + 868 = 994** changed values. Snapshot deltas do not
measure same-value stores.

`pr_slot_a_printer_page_apply`, supplied privately with this run's
hash-checked source, font, and pre-page, reports six cells, 192 modeled
stores, and 126 value changes. It predicts **14,080/14,080** native
immediate post-call page bytes. The private checker pins executable and
loaded-code hashes, the caller frame, source and font hashes, every
plane at each stop, the later remainder, and C output. A synthetic
test covers six invented glyphs in color 11, only planes 0/1/3,
unchanged neighbors, repeat-call idempotence, and a bounds rejection
without mutation. No source text, font bits, RAM, image, or page payload
is tracked.

## Reproduce locally

Prepare a fresh ignored run with `tools/oracle/prepare_run.sh`. Start the
private heavy debugger with `-break-start`, arm `BP 08DE:09C3`, then
`RUN`. At the menu select the pinned slot-A save. On the `09C3` stop,
arm `BP 0F2D:0626` and run; the next printer entry has the frame above.
Arm `BP 08DE:1858` for the immediate return. For the first-view stop,
clear intermediate breakpoints with `BPDEL *`, arm `BP 3582:3387`, and
run. At each stop, save RAM with `MEMDUMPBIN 0000 0000 A0000`. For
each plane, select read-map register 4 with `OUTP 3CE 04`, set plane
0..3 with `OUTP 3CF`, then `MEMDUMPBIN A000 03C0 DC0`; restore read
map 0 and selector 5 before resuming. Keep every dump and capture under
ignored `resources/`.

```sh
cmake -S . -B resources/build-slot-a-text-ui-six \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-text-ui-six --target \
  poolrad_slot_a_six_cell_printer_tests poolrad_slot_a_printer_page_probe
ctest --test-dir resources/build-slot-a-text-ui-six \
  -R '^poolrad_slot_a_six_cell_printer_tests$' --output-on-failure
python3 tools/replay/slot_a_six_cell_printer_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --run resources/oracle-runs/slot-a-text-ui-after-09c3-p9-003 \
  --probe resources/build-slot-a-text-ui-six/poolrad_slot_a_printer_page_probe
```

The audit's generated probe records stay in the ignored run.
