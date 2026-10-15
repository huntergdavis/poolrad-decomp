# Saved slot-A first-flush static page delta

This read-only audit compares the same-run EGA staging and page planes
before `RES_0464:1B52` and at its immediate return. It identifies
the first changed **address** and maps plane-byte changes to logical
indexed-pixel positions. It does not identify an intermediate
instruction stop or measure the palette at the immediate return.

Before analysis, `deja "Poolrad saved slot A first view A000 EGA plane
first changed offsets indexed RGB palette pre post static mapping"`
exceeded the bounded wait without a result. This audit reuses the
reviewed [first-row staging proof](SLOT_A_RES464_FIRST_STAGE_ROW.md),
the merged [all-dirty transfer](SLOT_A_U40C_ALL_DIRTY_TRANSFER_MODEL.md),
the [first-flush indexed region](SLOT_A_U40C_DIRTY_INDEXED_VIEW.md),
and the separate [stable first-view RGB join](SLOT_A_RGB_FRAME_PALETTE.md).
The latter proves the later stable frame's palette and 64,000 pixels,
not the palette timing at this immediate flush.

The ignored source `START.EXE` hashes to
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The private same-run entry and immediate-return 640-KiB RAM hashes are
`48becc9f9848794f232e6887a08edfd8b59d901833ccffbbbea4c336245080d7`
and
`db0fc49dad03cc5a45ececba4017f2eddf51d608f1c5993608334d63f777085f`.
The audit independently pins all sixteen 8,192-byte pre/post `A200`
and `A000` plane captures under ignored
`resources/oracle-runs/slot-a-view4-last6-compose-p5-001/`.
Entry video state is one, both selected page segments are `A000`,
and rows 16..103 are the 88 dirty rows.

| Same-run result | A200 staging | A000 page |
| --- | ---: | ---: |
| First changed offset, planes 0/1/2/3 | `03C3/03C3/03EB/03C3` | `03C3/03C3/03EB/03C3` |
| Changed bytes, planes 0/1/2/3 | 772/968/768/277 | 772/968/768/277 |
| Union of changed offsets | 968/968 dirty positions | 968/968 dirty positions |
| Changes outside dirty positions | 0 | 0 |

The first changed page byte `A000:03C3` is at mode-0D scanline
`y=24`, byte column three, logical `x=24..31`; the first changed
pixel is `(24,24)`. Plane two's first change `03EB` is one scanline
later, `y=25`. The entire first dirty row changes 88/88 indexed
pixels; all reached dirty spans change 7,667/7,744. Post-staging and
post-page bytes agree at all 3,872 dirty plane-byte positions. This
mapping emits no indices or RGB values.

The existing stable first-view RGB proof uses a separately measured
hardware palette and a later page snapshot. Without the immediate
flush's same-run palette or a finite `0AF8:16A1` CPU/page stop, this
audit makes no immediate-flush RGB parity, first-store-instruction,
or complete-frame claim. The separate first-page-byte C candidate is
held pending that stop; no strict full-C or identity count changes.

Reproduce with private inputs present:

```sh
python3 tools/replay/res464_first_flush_delta_audit.py \
  --run resources/oracle-runs/slot-a-view4-last6-compose-p5-001 \
  --exe resources/DOS/POOLRAD/START.EXE
```

Original media, RAM, EGA planes, and screenshots remain ignored under
`resources/`.
