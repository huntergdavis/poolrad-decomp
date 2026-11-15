# Synthetic AND store stub

The loaded `0052` stub used by the store-word call jumps to unit 25 `0C77`,
the store helper described in `U25_STORE_WORD_0C77.md`. The bounded adapter
authenticates the caller-supplied five-byte stub, requires the live call-frame
arguments to match destination `6E82` and the result derived from an explicit
synthetic source byte, and exposes the actual target segment. Invalid input
refuses without changes. Caller supplies immutable guest code and disjoint host
objects; the stub binding does not authenticate the target body.

The synthetic chain reaches `0C77` with destination `6E82` and result `0015`
for source byte `95`. The store helper has not executed. The source is not a
measured live `DS:6AB1` value; the earlier IF predicate and value 11 are
synthetic. Same-invocation provenance, native `345B` parity and a complete Up
route remain unverified.
