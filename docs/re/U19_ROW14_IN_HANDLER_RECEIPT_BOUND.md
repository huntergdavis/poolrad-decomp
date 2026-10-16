# U19 row-14 in-handler four-plane receipt bound

This source-only gate defines what one selected `A000:118F` handler receipt
can establish for the codeword row. It does not capture native backing bytes,
identify the first changing store, prove a displayed page, or join host RGB.
A bounded `deja` query for the selected U19 handler receipt and alias/no-op
semantics timed out without an indexed result. Reused checked-in research is
the [alternate-page crop source](U19_ROW14_ALTERNATE_PAGE_CROP_SOURCE.md),
[first-plane-change bound](U19_ROW14_FIRST_PLANE_CHANGE_SOURCE_BOUND.md),
[passive packet](U19_ROW14_PASSIVE_PACKET_V2.md), and
[capture integrity gate](U19_ROW14_PLANE_CAPTURE_INTEGRITY.md).

## Source and address lock

On merged main `f450a0ddda669779b7cbc35f9d5cecd3be026cad`, the in-handler
capture header hashes to
`fe232c91f52947c5df09c62d9e16c6ea4bafe9e439ebe802fc2d4af64de42d2c`;
the passive v2 validator hashes to
`bab29edfdc3c09a67a6e7aec41d9980b1c18fa7913255ce9599db4bfb712ac7e`.
The ignored DOS `START.EXE` lock and four exact original store-site hashes
are recorded in the [first-plane-change bound](U19_ROW14_FIRST_PLANE_CHANGE_SOURCE_BOUND.md).
The conditional row has 2,304 ordered guest byte-store candidates across
two pages. The first possible `0x118F` visits are ordinals 897/898 at
`RES_0709:+0445/+0451` (zero operand), followed by 899/900 at
`+0412/+0438` (glyph operand). They are possible CPU visits, not observed
native writes or plane changes.

## One selected receipt

The merged `observe_write` increments `write_count` for an armed handler
event, checks the exact guest linear target, byte width, supported handler,
resolved backing index, and stable observation state, then calls the original
write once. It copies four plane bytes at that same backing index before and
after the write. `finish` accepts only the matching successor and exactly one
captured write. The capture records a `uint32_t actual_index`; it carries no
complete host mapping or provenance by itself. The passive v2 validator
independently requires `handler_index == planar_index(host_map)` from the
supplied measured mapping.

No plane-byte delta is required for a valid receipt. If all four `before`
and `after` plane bytes match, the validator returns `changed_planes=()`;
that means **one recorded store with no observed value change**, not a
missing store. A missing, duplicate, wrong-target, wrong-width, wrong-frame,
wrong-successor, or unstable-state observation remains a separate failure.
The merged passive test exercises the empty delta; the merged capture test
exercises duplicate-write failure. A future native receipt must still bind
the selected ordinal and handler frame to the same run before claiming this
behavior occurred in the saved demo.

## Guest address versus backing alias

Under the conditional `A000/A000` page words, first- and second-page guest
stores can use the same `A000:118F` address at separate ordinals; later
stores can overwrite earlier results. This guest-address equality does not
replace a per-event measured host-map/index receipt. With `A000/A200` or
`A200/A000`, only one page's stores match the exact `A000:118F` filter.
`A200:118F` has the real-mode linear address of `A000:318F`, so an exact
`A000:118F` frame filter must not relabel it. Distinct visits to the same
site or backing word retain distinct ordinals and before/after pairs.

A [separate saved-slot-A `RES_0464:+1B52` transfer audit](SLOT_A_RES464_SECOND_FLUSH.md)
measures 88 dirty-row spans with 11 positions per row and plane, ending at
exclusive offset `0x1166`, `0x29` before `0x118F`.
That other run gives no U19 selected-byte receipt or backing state to import.
The supplied Python host-map model also admits integers beyond the pinned
host `uint32_t` backing-index domain; such an oversized map is a validator
domain lead, not an observed native index or a reason to truncate the
receipt. Keep the exact host ABI and supplied-map domains distinct.

Read-only reproduction, with no game media output:

```sh
sha256sum tools/replay/u19_row14_in_handler_capture.hpp \
  tools/replay/cold_start_codeword_row14_passive_snapshot_v2.py
python3 -m unittest tests.replay.test_cold_start_codeword_row14_passive_snapshot_v2
```

Status: **SOURCE_ONLY_RECEIPT_BOUND**. No first guest writer, first changed
plane, displayed page, host RGB, native parity, or full-frame credit follows
from this note.
