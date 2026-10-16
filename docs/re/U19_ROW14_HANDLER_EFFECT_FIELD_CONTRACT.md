# U19 selected handler effect field: local source contract

This is a source-only contract for a local opt-in C++ selected-event effect.
It does not add an emulator hook, change a packet wire schema, or authenticate
native execution. A bounded `deja` query for the finished-capture effect
field timed out after 12 seconds without an indexed result. The reused
checked-in evidence is the [selected receipt effect bound](U19_ROW14_SELECTED_RECEIPT_EFFECT_SOURCE.md),
[handler receipt bound](U19_ROW14_IN_HANDLER_RECEIPT_BOUND.md), and
[v2 pair join](U19_ROW14_V2_PAIR_JOIN.md).

## Source and ownership boundary

At main `2b1655f3a970506bb048eb7edc9088c93275881a`,
`tools/replay/u19_row14_in_handler_capture.hpp` hashes to
`fe232c91f52947c5df09c62d9e16c6ea4bafe9e439ebe802fc2d4af64de42d2c`.
Its `Capture` holds the four before and after plane bytes, one actual backing
index, the observed frame and VGA register/latch state, a write count, and a
stop reason. `finish` accepts one captured write at the selected successor.
`finished_capture` is already the opt-in event emitter's precondition.
The producer-handler adapter
(`4ec9b25dc6c988fb87275f71ca2928ad01830ec91f6bb9e418deac8946e40bd9`)
binds the existing eight selected identity fields to an audited source-only
recorder packet. Neither source supplies a live clock or authenticates native
coverage.

The pR v2 passive parser independently derives `changed_planes` from one
supplied four-byte before/after pair; the pC pair join carries that tuple after
checking all eight selected identity fields. Their packet schemas and source
files remain separately owned. pR gave direct relative CLEAR and pC gave an
exact-hash source PASS/CLEAR for the pre-edit contract. Both retain their own
consumer and packet ownership.

## Local C++ field

The opt-in `changed_planes_mask` byte is added to the C++ `SelectedEvent`
result.
After `finished_capture(capture)` succeeds, compare each complete plane byte
`capture.before.planes[i]` to `capture.after.planes[i]`; set bit `i` on
inequality for `i=0..3`. A value of zero is a **valid no-value-change
receipt** only when `EventEmitter.emitted` is true. Values 1..15 describe
changed planes for that one accepted selected write. They do not identify the
first value-changing store among the 2,304 conditional candidate visits.
The witness bit 0x20 staying equal does not imply equal complete bytes.

A missing write, duplicate write, wrong target/width/frame/successor,
unstable observation state, invalid backing, disabled emitter, or invalid
selected identity must not emit an effect receipt. A default-initialized
zero mask with `emitted=false` is **absent**, never a no-op receipt.
The emitter must derive the mask from the already copied `Capture`; no
second backing read and no caller-supplied mask are allowed. The original
host write must retain its existing single execution and stop behavior.

This field is local C++ output. The existing version-2 producer packet has an
exact eight-key `selected_event` schema; adding a ninth wire key would fail
its strict audit and needs a separate versioned contract with pC and pR.
No host timestamp, page identity, RGB value, or backend event is inferred
from the mask or from `candidate_ordinal`.

## Synthetic strict test gate

The focused test uses synthetic backing only. Its distinguishing cases are:

- One finished unchanged four-plane word emits mask 0 and exactly one
  selected event.
- Finished writes change planes 0 and 2 or planes 1 and 3 while bit 0x20
  stays equal across all four bytes; they emit masks 0x05 and 0x0A without
  re-reading backing memory.
- Missing and duplicate writes, wrong successor, and state-copy mutation
  leave `emitted=false`; a wrong target does too. None can be mistaken
  for a valid zero mask.
- Disabled or already emitted output and an invalid audited identity cannot
  mint another effect or change the original host-write count.

The field and tests remain local pending PM review. They claim no first
changed store, native frame, selected display page, or RGB parity.
