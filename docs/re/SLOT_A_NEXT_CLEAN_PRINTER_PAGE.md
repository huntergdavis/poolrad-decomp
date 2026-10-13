# Saved slot-A first view: next reached clean-page printer call

## Boundary and reused evidence

This note isolates the next complete `RES_0709:0626` string-printer call
after the [first measured print](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md) and before
the saved first-view return. It accounts for **14** of the 1,036 clean
plane-byte changes left by that first call. It does not establish a complete
first-view renderer or full-C parity for either resident routine.

Before debugging, `deja "Poolrad slot A after RES_0709 printer 1036 clean plane
bytes next writer first view"` found no indexed match. This audit reuses the
PR #327 private replay protocol and bounded C page projection, the
[RES_00BA:08EF field-display recovery](RESIDENT_FIELD_08EF.md), and the
[resident 0767 caller model](RESIDENT_LOAD_PIECES_0767.md). It does not change
their behavior or claim saved-route branch outcomes outside the observed
call frames.

## Hash-locked call and parent

The ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
`RES_00BA:08EF` maps to expanded image `[0x0148F,0x01513)` (132 bytes,
SHA-256 `c4d1ffe64d255ff80f1f812284a6145df3b56815fdf4a4aadff777798e32cb0d`).
Its printer far call at `RES_00BA:0968`, image `[0x01508,0x0150D)`, hashes to
`04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23`.
The live relocated instruction at `08DE:0968` hashes to
`cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`
and targets `0F2D:0626`. The printer's 129-byte expanded image span
`[0x076B6,0x07737)` hashes to
`b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`.

The private replay stops in order at first printer return `08DE:0814`, next
printer entry `0F2D:0626`, next printer return `08DE:096D`, and first-view
return `3582:3387`. At the next entry, `SS:SP=1D16:3C96`; the far return
points to `08DE:096D`, and arguments are source `1D16:3DA6`, color 10,
row 4, column 34. The Pascal source length is one; the length-plus-byte
SHA-256 is `f16ba6fa61da3398815be2a6c0f7cb1351982dbcc6c64bbeb9b65f672a8b102a`.
The live 1,416-byte font hash is
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
Both EGA page segments are `A000`, and the display mode is EGA. The
`RES_00BA:08EF` parent frame returns to `RES_00BA:0878`, the reached
`0767` field caller. This frame proves the display callee and printer were
reached; it does not independently record the `08EF` internal branch's
earlier operand values.

## Same-run plane contribution

The fresh ignored run is `resources/oracle-runs/slot-a-next-clean-p5-001/`.
Each page snapshot is four 3,520-byte planes for `A000:[0x03C0,0x1180)`;
the table hashes their 14,080-byte concatenation.

| Stop | Four-plane SHA-256 | RAM SHA-256 |
| --- | --- | --- |
| first printer return | `144f74d0e9111b44175354a75f6cf0ed462be479c70fbb23fb5452c60dd842c0` | `9f5e61575b69399cdf1f4a5c8d4c1a8e9a370872a3d2f36ae805e9f0a9eea05d` |
| next printer entry | `144f74d0e9111b44175354a75f6cf0ed462be479c70fbb23fb5452c60dd842c0` | `bd456b051f26cad2b77a4760e29528c22cc03e7f98f56716a2b05a0228c32e9e` |
| next printer return | `889c0ee8008089ef2feafba5c2da1e5dfab0b509422d92aa76f863aabf6fefb8` | `37a1a6a8d113268bb24b3df9d2a21aa547bd7c722f135a6e3b4ac54139595555` |
| first-view return | `d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b` | `e0d8642621bdc1df657197485fdb7ddaf153eb8f8eb8e154a99f2cf995096a92` |

The page has **zero value changes** between the two printer stops. The
second call targets one cell at row 4, column 34 (eight scanlines, four
planes, 32 plane-byte targets). The complete call changes 14 plane bytes,
seven each on planes 1 and 3; all lie outside the first flush's dirty
columns 3..13 and the first printer's columns 17..30. All 14 retain their
post-call values at first-view return. The other **1,022** changes occur
later, are disjoint from this cell, and remain a separate display gate.
The total after the first printer remains 14 + 1,022 = 1,036.

The existing `pr_slot_a_printer_page_apply` predicts the next call's
entire immediate post-call page **14,080/14,080 bytes**, using the ignored
font, one-byte Pascal source and pre-call page. Its report is one cell,
32 modeled stores and 14 value changes. A new synthetic test exercises
one cell at row 4, column 34 with color 10, selective planes, unchanged
neighbors and a repeated same-value call. This is bounded page-output
parity for one reached call; it adds no full-routine or game-identity count.

```sh
cmake -S . -B resources/build-next-clean \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-next-clean --target \
  poolrad_slot_a_printer_page_tests poolrad_slot_a_printer_page_probe
ctest --test-dir resources/build-next-clean \
  -R '^poolrad_slot_a_printer_page_tests$' --output-on-failure
python3 tools/replay/slot_a_next_printer_page_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --run resources/oracle-runs/slot-a-next-clean-p5-001 \
  --probe resources/build-next-clean/poolrad_slot_a_printer_page_probe
```

All original executables, save data, text, font bytes, RAM and EGA planes
remain under ignored `resources/`.
