# Saved slot-A first view: next row-five printer cell

After the row-five column-34 printer return measured in
[PR #335](https://github.com/huntergdavis/poolrad-decomp/pull/335), the saved
slot-A route reaches another `RES_0709:0626` printer call. Its page is
unchanged at entry. The call prints one cell at row **5**, column **38** and
changes **14** EGA plane bytes. The existing bounded C printer-page model
matches its entire immediate 14,080-byte page. This is one call's page-output
parity, not a complete first-view or full-routine translation.

Before tracing, `deja "Poolrad slot A row5 one cell printer return next EGA
page mutation first changed bytes RES_00BA 0968"` returned no focused
session. The merged [printer model](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md) and
the pending [row-five call](https://github.com/huntergdavis/poolrad-decomp/pull/335)
provided route leads; this run independently measured the next frame and
page boundaries.

## Hash-pinned call path

The fresh ignored run is
`resources/oracle-runs/slot-a-text-ui-after-row5-p9-005/`, prepared by
`tools/oracle/prepare_run.sh`. DOS v1.3 `START.EXE`, `GAME.OVR`, and the
slot-A save SHA-256 values are respectively
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private heavy DOSBox-X binary hashes to
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

At `08DE:1858`, `[BP+06]=0`; the replay then reached the column-34
printer return `08DE:096D`. The next reached printer entry was
`0F2D:0626`, before first-view return `3582:3387`.

| Original expanded image span | Length | SHA-256 | Live meaning |
| --- | ---: | --- | --- |
| `[0x1457,0x145A)` | 3 | `e72bebba602f7eb4edd3783632f297e7b0bf432aa0b0438a34908168cce490a4` | near call `RES_00BA:08B7 → 0973`, return `08BA` |
| `[0x1513,0x1569)` | 86 | `a5093db76117998bf62e1d6a3b0ca1816f6109dd7d3bca570070a09c0f73ebd0` | bounded `RES_00BA:0973..09C8` body |
| `[0x155E,0x1563)` | 5 | `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23` | far call `RES_00BA:09BE → RES_0709:0626`, return `09C3` |
| `[0x76B6,0x7737)` | 129 | `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed` | printer body |

The loaded near-call hash is unchanged. The loaded `08DE:0973..09C8`
body hash is
`c7577505151147a243592d02b42335233680a27273dc5df3f427d4e778a3c3d3`;
the relocated far call at `08DE:09BE..09C2` hashes to
`cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`
and targets `0F2D:0626`. The loaded printer span hashes to
`8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`.

At printer entry, `SS:SP=1D16:3D94`. Its seven frame words give far
return `08DE:09C3`, source `1D16:3DA2`, color **10**, row **5**, column
**38**. The parent frame returns to `08DE:08BA`. The Pascal source length
is **1**; its length-plus-content SHA-256 is
`06f647b157b8557a12979ba04cf5ba222b9747cf5bbe82f24a48e25c0c508042`.
The live 1,416-byte font SHA-256 is
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
`DS:4A1E=1` selects EGA; both page segment words are `A000`.

## Same-run page effect

Each page concatenates four 3,520-byte EGA planes from
`A000:[0x03C0,0x1180)`. The baseline at `08DE:096D` equals the next
printer-entry page byte for byte.

| Stop | 640 KiB RAM SHA-256 | 14,080-byte page SHA-256 |
| --- | --- | --- |
| `08DE:096D` baseline | — | `721b9e030448fa2c363585f0726d469a2a4cdd61d61d2fecae6195480219dfe0` |
| `0F2D:0626` entry | `d8e994f159c9d4ad6f59ef3cde511b769e1b87a2b18ca7c04ced592c46b7d0e5` | `721b9e030448fa2c363585f0726d469a2a4cdd61d61d2fecae6195480219dfe0` |
| `08DE:09C3` immediate return | `b65100d911d1fe2d457fd47707046d11e63271474490b1e6885139d43b304244` | `304a1a91d14d0e80bcb5c97053ab6618b6b8a7ab5d463977521e8c448968dd86` |
| `3582:3387` first-view return | — | `d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b` |

The cell occupies column **38**, scanlines **40–47**. The modeled
eight rows × four planes give **32** plane-byte stores. Immediate snapshots
show **14** changed values: seven each in planes 1 and 3, at `A000`
offsets `0666, 068E, 06B6, 06DE, 0706, 072E, 0756` (scanlines 40–46).
All 14 values persist at first-view return. That later page differs at
**840** other bytes; their writers are not assigned here. Snapshot deltas
do not count same-value stores.

Using this run's ignored source, font, and entry page, the existing
`pr_slot_a_printer_page_apply` reports one cell, 32 modeled stores, and
14 value changes. Its output matches **14,080/14,080** native immediate
post-call page bytes. The synthetic test uses an invented glyph and guards
the adjacent cell, previous row, repeated call, and out-of-bounds call.
Neither comparison proves complete first-view C parity.

## Reproduce locally

Prepare a fresh ignored run and start the hash-pinned heavy debugger. At
the slot prompt, arm `BP 08DE:1858` before selecting A. After that stop,
arm `BP 08DE:096D`; capture the baseline page at its next hit. Arm
`BP 0F2D:0626` and `BP 3582:3387`; the printer entry arrives first.
Capture entry RAM with `MEMDUMPBIN 0000 0000 A0000` and its page. Arm
`BP 08DE:09C3` for the immediate return; capture RAM and page. Finally
arm `BP 3582:3387` and capture the first-view page. To capture each plane,
select Graphics Controller register 4 with `OUTP 3CE 04`, set its read map
0..3 with `OUTP 3CF`, and run `MEMDUMPBIN A000 03C0 DC0`. Restore read
map 0 and register selector 5 before resuming. Keep dumps and captures in
ignored `resources/`.

```sh
cmake -S . -B resources/build-slot-a-row5-col38 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-row5-col38 --target \
  poolrad_slot_a_row5_col38_printer_tests poolrad_slot_a_printer_page_probe
ctest --test-dir resources/build-slot-a-row5-col38 \
  -R '^poolrad_slot_a_row5_col38_printer_tests$' --output-on-failure
python3 tools/replay/slot_a_row5_col38_printer_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --run resources/oracle-runs/slot-a-text-ui-after-row5-p9-005 \
  --probe resources/build-slot-a-row5-col38/poolrad_slot_a_printer_page_probe
```

The audit checks the original and loaded spans, frame, source/font hashes,
every captured plane, persistence, and exact C output. Generated probe
records remain in the ignored run.
