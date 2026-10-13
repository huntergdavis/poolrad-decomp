# Saved slot-A first view: next row-six eight-cell printer call

After the row-five column-38 printer return measured in
[PR #338](https://github.com/huntergdavis/poolrad-decomp/pull/338), the saved
slot-A route reaches another `RES_0709:0626` printer entry. Its entry page
equals the prior return page. This call prints eight cells at row **6**,
columns **17–24**, changing **168** EGA plane bytes. The existing bounded
C printer-page model predicts the entire immediate 14,080-byte page. This
establishes page-output parity for one reached call, not a complete static
frame or full-routine translation.

Before tracing, `deja "Poolrad slot A after RES_00BA 09BE row five column
38 printer return next EGA page writer"` gave no focused match. The merged
[printer model](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md),
[PR #334](https://github.com/huntergdavis/poolrad-decomp/pull/334)'s earlier
use of `RES_00BA:1853`, and PR #338's immediate post-page were route leads.
This fresh run independently measured the parent path, caller frame, and
page boundary for the new invocation.

## Original and loaded path

The ignored run is `resources/oracle-runs/slot-a-text-ui-after-col38-p9-006/`,
prepared by `tools/oracle/prepare_run.sh`. DOS v1.3 `START.EXE`,
`GAME.OVR`, and slot-A save SHA-256 values are respectively
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private heavy DOSBox-X binary hashes to
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

The replay stopped at `08DE:1858` for the earlier call, then at PR #338's
`08DE:09C3` return. The next reached printer entry was `0F2D:0626`,
before first-view return `3582:3387`.

| Original expanded image span | Length | SHA-256 | Call meaning |
| --- | ---: | --- | --- |
| `[0x13C8,0x13CB)` | 3 | `101742b95d662bb42996389a5744d3371fb252318a3e91c03de2d6e0f01469d3` | near `RES_00BA:0828 → 1816`, return `082B` |
| `[0x23B6,0x2492)` | 220 | `47a999a7d5c19202877fb35003099aab45dac77ce4bb95e93d132020a2373918` | bounded `RES_00BA:1816..18F1` body |
| `[0x23F3,0x23F8)` | 5 | `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23` | far `RES_00BA:1853 → RES_0709:0626`, return `1858` |
| `[0x76B6,0x7737)` | 129 | `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed` | printer body |

The loaded near-call hash is unchanged. The loaded
`08DE:1816..18F1` body hashes to
`35cdb1f71e2bc52f5f2d377d647594481f50921a7c09a0aed1b01057aa6087ac`.
The relocated far call `08DE:1853..1857` hashes to
`cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`
and targets `0F2D:0626`. The loaded printer body hashes to
`8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`.

At printer entry, `SS:SP=1D16:3E92`. Its frame gives return
`08DE:1858`, source `44D6:0008`, color **11**, row **6**, column
**17**. The parent frame returns to `08DE:082B`. The Pascal source
length is **8**; its length-plus-content SHA-256 is
`55d8fce47b663249e52bee88b9f200fac4b123e6f93a9c78dcd39566dca69cc0`.
The live 1,416-byte font SHA-256 is
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
`DS:4A1E=1` selects EGA and both page segments are `A000`.

## Same-run EGA contribution

Each page concatenates four 3,520-byte EGA planes from
`A000:[0x03C0,0x1180)`. The PR #338 return page and this call's entry
page are byte-identical. This snapshot equality does not rule out
intervening writes followed by restoration.

| Stop | 640 KiB RAM SHA-256 | 14,080-byte page SHA-256 |
| --- | --- | --- |
| `08DE:09C3` baseline | `a3cca0298f6be949aa02790da5733f50b105b860a67df558e3408503bef8c4c1` | `304a1a91d14d0e80bcb5c97053ab6618b6b8a7ab5d463977521e8c448968dd86` |
| `0F2D:0626` entry | `fffde31cfe190e68a85452a9ac7f5428905dbf2b63c7a1446560a47b2bcdcd3d` | `304a1a91d14d0e80bcb5c97053ab6618b6b8a7ab5d463977521e8c448968dd86` |
| `08DE:1858` immediate return | `ee88688e25e9be541e43adba0df579c565e35ec30747130978aeb7ca5abeb5e6` | `421bb334ebb286c22f254a05d1d8e872c6a0c321714c248c2e1c16149ddb21d5` |
| `3582:3387` first-view return | — | `d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b` |

Eight cells × eight scanlines × four planes imply **256** modeled
plane-byte stores. Immediate snapshots differ at **168** positions:
all eight columns in scanlines **48–54** of planes **0, 1, and 3**;
the affected `A000` offsets range from `0791` to `0888`. Scanline
55 and plane 2 have no net value change. All 168 values persist at
first-view return. Another **672** page bytes differ later; this note
does not assign their writers. Snapshot deltas do not count same-value
stores.

Given this run's ignored source, font, and entry page, the existing
`pr_slot_a_printer_page_apply` reports eight cells, 256 modeled stores,
and 168 changed values. Its output matches **14,080/14,080** native
immediate post-call page bytes. The public synthetic test uses an
invented glyph and checks the adjacent cells, prior row, repeated call,
and an out-of-bounds call without mutation. This adds no full-C count.

## Reproduce locally

Prepare a fresh ignored run and start the hash-pinned heavy debugger.
Before choosing slot A, arm `BP 08DE:1858`. At that first stop, arm
`BP 08DE:09C3` and capture the baseline RAM and page. Arm
`BP 0F2D:0626` plus `BP 3582:3387`; the printer entry arrives first.
Capture entry RAM and page, then arm `BP 08DE:1858` for the immediate
return and capture both. Finally arm `BP 3582:3387` and capture the
first-view page. `MEMDUMPBIN 0000 0000 A0000` captures private RAM.
For each EGA plane, select Graphics Controller register 4 with
`OUTP 3CE 04`, select read map 0..3 with `OUTP 3CF`, and run
`MEMDUMPBIN A000 03C0 DC0`. Restore read map 0 and selector 5 before
resuming. All dumps and screenshots remain under ignored `resources/`.

```sh
cmake -S . -B resources/build-slot-a-row6-eight \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-row6-eight --target \
  poolrad_slot_a_row6_eight_cell_printer_tests poolrad_slot_a_printer_page_probe
ctest --test-dir resources/build-slot-a-row6-eight \
  -R '^poolrad_slot_a_row6_eight_cell_printer_tests$' --output-on-failure
python3 tools/replay/slot_a_row6_eight_cell_printer_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --run resources/oracle-runs/slot-a-text-ui-after-col38-p9-006 \
  --probe resources/build-slot-a-row6-eight/poolrad_slot_a_printer_page_probe
```

The audit pins the original and loaded path, frame, source and font
hashes, all captured planes, persistence, and exact C output. Probe
records stay in the ignored run.
