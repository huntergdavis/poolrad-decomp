# Synthetic AND store call

`U1C_AND_OR_0DEF.md` documents the store-word far call at `0E55`, returning to
`0E5A`. The bounded adapter authenticates the supplied five-byte call,
requires the live arguments to be destination `6E82` and the result derived
from the explicit synthetic source byte, and rejects stack/code overlap and
wrapping frames. It exposes the actual stub segment and call frame. Caller
supplies immutable guest code and disjoint host objects; invalid inputs refuse
before mutation.

The synthetic chain reaches stub `0052` with destination `6E82` and result
`0015` for source byte `95`. The stub and store helper have not executed. The
byte is not a measured live `DS:6AB1` value; the earlier IF predicate and
value 11 are synthetic. Same-invocation provenance, native `345B` parity and a
complete Up route remain unverified.
