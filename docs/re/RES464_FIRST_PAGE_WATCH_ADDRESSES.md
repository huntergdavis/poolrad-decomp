# Saved slot-A: first-row page-store watch addresses

The [address table](res464_first_page_watch_addresses.csv) gives the 44
plane/address pairs for the first dirty row at the saved first-view
`RES_0464:1B52` flush. It is a **static watch plan**, built from
hash-locked existing captures. Its change flags compare flush entry
with immediate return. They do not establish which instruction made
each change or prove the held first-page-byte C candidate.

Before this static task, `deja "Poolrad saved slot A RES_0464 1D27
A200 A000 03C3 first page store runtime Move 16A1 watch address
planes"` found no indexed session. Reused the merged
[all-dirty transfer audit](SLOT_A_U40C_ALL_DIRTY_TRANSFER_MODEL.md)
and the PM-approved local static plane-delta audit at `4e3ac645`.
The separate C candidate at `664ca1a260553ce7042e2262ca402cfc4e681f51`
remains local HOLD pending direct native proof.

## Pinned route and input

Ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The resident post-stage `+1D27..+1DB5` file span
`[0x63A0,0x642E)` hashes to
`83acb4eddfdaa0e6323c621a26c691bcef55e6ed7412e3cc8c3a678fbaf77f9a`.
Runtime `0AF8:1685..16A8`, file `[0xC2FB,0xC31E)`, hashes to
`1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972`.
The exact +1DA0 far call invokes that Move routine. Its
`+1696` comparison selects forward `REP MOVSB` at `+16A1`
when the source and destination offsets are both `0x03C3`.

The ignored same-run directory is
`resources/oracle-runs/slot-a-view4-last6-compose-p5-001/`.
Flush-entry RAM SHA-256 is
`48becc9f9848794f232e6887a08edfd8b59d901833ccffbbbea4c336245080d7`;
immediate-return RAM SHA-256 is
`db0fc49dad03cc5a45ececba4017f2eddf51d608f1c5993608334d63f777085f`.
The [read-only audit](../../tools/replay/res464_first_page_watch_table.py)
pins those two 640-KiB files and all sixteen 8-KiB
`A200`/`A000` plane captures by SHA-256. It verifies video state 1,
rows 0..15 clean, row 16 dirty, source words 1352..1395, low offset 2,
and eleven-byte staging and page extents beginning at `0x03C3`.

## Static watch targets

The first CPU source span is `A200:03C3..03CD`; the corresponding
page span is `A000:03C3..03CD`. Four plane rows share each CPU address
in the table. At the first address, the entry-to-return page comparison
changes planes 0, 1 and 3, while plane 2 retains its value. Across all
eleven offsets the respective plane change counts are **11/11/0/11**.
All 44 final staging plane-byte positions equal the corresponding final
page positions. A plane-2 zero net change does not prove that no write
occurred; a same-value write is possible.

The proposed finite native breakpoint is **before `0AF8:16A1`**,
with a separate after-one-byte stop or write watch at the page CPU
address `A000:03C3`. Record CS:IP, DS:SI, ES:DI, CX, DF, SS:SP,
caller words, map mask, Graphics Controller write mode, and all four
source/destination planes on both sides of that one byte. Debugger
watchpoint granularity and plane behavior must be measured. Existing
entry and immediate-return captures supply no pre-`+16A1` CPU
tuple or per-instruction page state.

This table makes no page-store instruction parity, palette/RGB timing,
visible-frame, strict full-C, or game-identity claim. The table has
only addresses and net-change flags, no game bytes or pixels. Its
SHA-256 is
`83a79277c435305402f7f1ec9f5388c3c05a6183e0d74a137bd404f88ff585f0`.

Reproduce with the ignored inputs already present:

```sh
python3 tools/replay/res464_first_page_watch_table.py
```

`--emit-csv` prints the same address-only table for comparison.
