# Second AND operand reader prefix

The raw unit 25 slot-value reader documented in `U25_SLOT_VALUE_0136.md`
begins at `0136` with a BP save and ten-byte local-frame allocation. The
bounded adapter authenticates the caller-supplied six-byte reference, rejects
stack/code overlap and wrapping frame offsets, and exposes CPU/RAM state at
`013C`. Caller supplies immutable guest code and disjoint host objects.

The synthetic chain reaches this stop before the reader accesses its operand
tables. `C04F` remains undereferenced; the IF predicate and earlier value 11
are synthetic. Same-invocation provenance, native `345B` parity and a complete
Up route remain unverified.
