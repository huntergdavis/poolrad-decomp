# U19 row-14 selected receipt effect bound

This source-only gate separates one valid selected handler write with no
observed four-plane byte change from one valid selected write that changes a
plane byte. Neither outcome identifies the first change across the 2,304
conditional row-store candidates. A bounded `deja` query for U19 selected
handler no-op and first-change policy timed out after 12 seconds without an
indexed result. Reused checked-in work is the [receipt bound](U19_ROW14_IN_HANDLER_RECEIPT_BOUND.md),
[first-change source bound](U19_ROW14_FIRST_PLANE_CHANGE_SOURCE_BOUND.md),
[candidate coverage gate](U19_ROW14_CANDIDATE_COVERAGE_GATE.md), and
[v2 pair join](U19_ROW14_V2_PAIR_JOIN.md).

## Exact source fields

At merged main `0d0d24ea9e9905dcd1341d377fbefd8abf2cfe11`, p5's
`tools/replay/u19_row14_in_handler_capture.hpp` hashes to
`fe232c91f52947c5df09c62d9e16c6ea4bafe9e439ebe802fc2d4af64de42d2c`.
`Capture` stores `write_count`, `actual_index`, and four `before.planes` and
`after.planes` bytes. `finish` accepts exactly one captured write and the
expected successor; a missing or duplicate write fails. Its `SelectedEvent`
stores `candidate_ordinal` (1..2304) and the selected `pc/successor_pc`, but
no plane bytes, delta flag, or host event clock. The p5 producer-to-handler
adapter hashes to
`4ec9b25dc6c988fb87275f71ca2928ad01830ec91f6bb9e418deac8946e40bd9`;
it copies the audited selected identity into that event without re-reading
backing memory.

The pR passive v2 validator hashes to
`bab29edfdc3c09a67a6e7aec41d9980b1c18fa7913255ce9599db4bfb712ac7e`.
It derives `changed_planes` from the four supplied before/after bytes and
accepts the empty tuple for equal bytes. The separate pC v2 pair join carries
those supplied bytes and the derived tuple after checking the eight selected
identity fields. Both are source-only supplied-packet contracts; matching
labels and bytes do not authenticate a native run.

| Finished selected capture | Four-plane comparison | Bounded result |
| --- | --- | --- |
| Exactly one accepted write | All four bytes equal | One valid no-value-change receipt; `changed_planes=()` in the passive validator. |
| Exactly one accepted write | Any plane byte differs | One selected value-changing receipt; the differing plane indices identify this receipt only. |
| Missing, duplicate, wrong target/frame/successor, or unstable copy | Any bytes | Invalid receipt; absence of a delta cannot be called a no-op. |

The owned recorder key is `SelectedEvent.candidate_ordinal`: it counts a CPU
candidate-PC visit, including stores to other `ES:DI` destinations. It is
not a plane-change count or host timestamp. The selected event currently has
no effect field; adding one would require a separately reviewed producer
contract that derives it from the **finished** `Capture` and keeps the
four-byte packet provenance. The current note adds no such field.

## First-change stop

The conditional original drawer has 2,304 candidate visits; the earliest
possible `A000:118F` witness is ordinal 897 or 898 depending on page words.
The guest sites are `RES_0709:+0412/+0438/+0445/+0451`; their exact
START image offsets and hashes are pinned in the
[first-change source bound](U19_ROW14_FIRST_PLANE_CHANGE_SOURCE_BOUND.md).
One selected receipt, changed or unchanged, cannot rule out value changes
at earlier candidate ordinals, other offsets, or the other page. A claim
about the *first* changed plane byte within this conditional route requires
authenticated complete same-run candidate order and valid resolved backing
before/after observations for every preceding reached candidate. A failed,
missing, or unobserved earlier receipt leaves the first-change result open.
The current hook captures only one selected word, while the public recorder,
packet, and pair join remain source-only. No saved-run page state, live
writer, or host RGB follows from this comparison.

Focused source-only check:

```sh
sha256sum tools/replay/u19_row14_in_handler_capture.hpp \
  tools/replay/u19_row14_producer_handler_adapter.hpp \
  tools/replay/cold_start_codeword_row14_passive_snapshot_v2.py
python3 -m unittest tests.replay.test_cold_start_codeword_row14_passive_snapshot_v2
```

Status: **SOURCE_ONLY_SELECTED_RECEIPT_EFFECT**. No native first-change,
displayed page, RGB, C parity, or full-frame credit is asserted.
