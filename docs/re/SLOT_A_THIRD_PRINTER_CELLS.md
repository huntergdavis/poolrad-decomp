# Saved slot-A first view: third reached printer call

This bounded TEXT/UI trace follows the merged
[second printer-call audit](SLOT_A_NEXT_CLEAN_PRINTER_PAGE.md) to the next
`RES_0709:0626` invocation before first-view return. It attributes **28 of
the 1,022** later changed EGA plane bytes to a two-cell print. The other
**994** remain unattributed. The existing C page model matches this call's
immediate 14,080-byte output. Neither the full first view nor the complete
resident printer is translated by this work; no full-C routine or identity
count is added.

Before tracing, `deja "Poolrad slot A after RES_00BA 096D next text UI
printer glyph 1022 clean page changes"` returned no focused session.
The [first measured print](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md),
[merged second print](SLOT_A_NEXT_CLEAN_PRINTER_PAGE.md), and
[font/printer recovery](FONT_8X8D_201.md) supplied the previously reviewed
call ABI and page capture method. The third call's stack, pre/post page
and return state below were measured afresh.

## Pinned original and live route

The ignored run is `resources/oracle-runs/slot-a-text-ui-after-096d-p9-002/`,
prepared and post-verified with `tools/oracle/prepare_run.sh`. Its DOS v1.3
`START.EXE`, `GAME.OVR`, and slot-A save SHA-256 values are
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private heavy DOSBox-X executable hashes to
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

The original `RES_00BA:09BE..09C2` far call is expanded image
`[0x0155E,0x01563)`, five bytes, SHA-256
`04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23`.
Its numeric target is `RES_0709:0626`. The live relocated call at
`08DE:09BE..09C2` hashes to
`cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`.
The already reviewed printer body has original expanded span
`[0x076B6,0x07737)` (129 bytes), SHA-256
`b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`;
its loaded span at `0F2D:0626..06A6` hashes to
`8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`.

The third printer entry `0F2D:0626` has `SS:SP=1D16:3D94` and a far
return to `08DE:09C3`. Its source pointer is `1D16:3DA2`, Pascal length
**2**, length-plus-content SHA-256
`fba5fe6efee9c741299411a77a7aa9e35505c7459f393499a6a3bf2f6beb4b32`.
The remaining arguments are color **10**, row **4**, column **37**. The
live 1,416-byte font buffer hashes to
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
`DS:4A1E=1` selects EGA and both page segment words are `A000`. The
guest returned to `08DE:09C3`, then reached the first-view return at
`3582:3387`. These stops establish this invocation, not the upstream
choice of characters or behavior of other printer paths.

## Same-run page contribution

Each page snapshot concatenates four 3,520-byte EGA planes from
`A000:[0x03C0,0x1180)`. The third call's pre-page hash equals the
merged second call's post-page hash from a separate replay.
Boundary equality does not rule out intervening writes and restoration.

| Stop | 640 KiB RAM SHA-256 | 14,080-byte page SHA-256 |
| --- | --- | --- |
| third printer entry | `88f846a1401c0879dca04ff74b5cb25bc9776861e12a8f402f70e44f4638f12b` | `889c0ee8008089ef2feafba5c2da1e5dfab0b509422d92aa76f863aabf6fefb8` |
| immediate third return | `d06762ea7a5f6453db3f1225590a764c67977edfcc2cd644d234bd21dc0497b1` | `00a5a7693604b8f8487ced7ab081c04a3ba5dffd2b8f0b749c9ff80919267d96` |
| first-view return | `d5da21fea8d8c74a20ae3415873d38aaade64f714aac05f369e71d066b16b0b1` | `d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b` |

The two cells occupy columns **37–38**, scanlines **32–39**. The
printer's EGA path models eight rows times two columns times four
planes, or **64 plane-byte stores**. Native pre/post snapshots differ
at **28** addresses: 14 in plane 1 and 14 in plane 3, all at the two
columns on scanlines 32–38. Every changed value is still present at the
first-view return; the later snapshot has no net change in these cells. The later page delta has **994** changed plane bytes outside these cells. Therefore the
post-second-printer remainder is **28 + 994 = 1,022** value-changing
bytes for this page rectangle. The final four-plane hash matches the
previously published first-view state. Snapshot deltas do not measure
same-value stores.

`pr_slot_a_printer_page_apply`, supplied privately with this run's
hash-checked source, font and pre-page, reports two cells, 64 modeled
stores and 28 value changes. It predicts **14,080/14,080** native
post-call page bytes. A new synthetic test covers two different glyphs
in color 10, only planes 1 and 3, unchanged neighbors, repeat-call
idempotence, and an out-of-bounds two-cell request without mutation.
The private checker pins executable and loaded-code hashes, the live
argument frame, source and font hashes, all four planes at each stop,
persistence, and C output. No game text, font bits, RAM, image, or
page payload is tracked.

## Reproduce locally

Use the ignored replay prepared above. In the heavy DOSBox-X debugger,
arm `BP 08DE:096D` and `BP 3582:3387` before selecting slot A. At
`08DE:096D`, arm `BP 0F2D:0626`. The next printer entry is this call;
arm its observed return `BP 08DE:09C3`. At entry, return, and first-view
return, capture `MEMDUMPBIN 0000 0000 A0000`. For each four-plane page
sample, select read-map register 4 with `OUTP 3CE 04`, set plane 0..3 via
`OUTP 3CF`, and dump `MEMDUMPBIN A000 03C0 DC0`; restore read map 0 and
selector 5 before resuming. Store every capture under ignored `resources/`.

```sh
cmake -S . -B resources/build-slot-a-text-ui-third \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-text-ui-third --target \
  poolrad_slot_a_third_printer_cells_tests poolrad_slot_a_printer_page_probe
ctest --test-dir resources/build-slot-a-text-ui-third \
  -R '^poolrad_slot_a_third_printer_cells_tests$' --output-on-failure
python3 tools/replay/slot_a_third_printer_cells_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --run resources/oracle-runs/slot-a-text-ui-after-096d-p9-002 \
  --probe resources/build-slot-a-text-ui-third/poolrad_slot_a_printer_page_probe
```

The audit's generated probe records stay in the ignored run. This work
uses no original game data in tracked tests or documentation.
