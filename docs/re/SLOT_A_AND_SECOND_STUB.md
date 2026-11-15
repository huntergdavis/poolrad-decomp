# Second AND operand stub

The caller-authenticated unit 25 `0025` stub is a far jump to offset `0136`,
the raw slot-value reader documented in `U25_SLOT_VALUE_0136.md`. The bounded
adapter follows the live target segment and exposes CS:IP while preserving the
call frame. It rejects a mismatched stub without mutation; the caller supplies
immutable guest code and disjoint host objects. This binding alone does not
authenticate the target body.

The synthetic AND chain reaches the reader with slot index two and the `C04F`
descriptor. The reader does not execute and `C04F` remains undereferenced.
The IF predicate and earlier value 11 are synthetic; same-invocation provenance,
native `345B` parity and a complete Up route remain unverified.
