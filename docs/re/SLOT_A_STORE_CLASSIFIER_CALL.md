# Synthetic AND store classifier call

The raw store-word helper in `U25_STORE_WORD_0C77.md` begins with a two-byte
local frame, pushes its destination and calls the classifier at `06FB`, with
return `0C84`. The bounded adapter authenticates the supplied 13-byte prefix
and call target, requires destination `6E82` and a result derived from the
explicit synthetic source byte, and rejects stack/code overlap and wrapping
frames. It exposes the live classifier frame and prologue flags. Caller
supplies immutable guest code and disjoint host objects.

The synthetic AND chain reaches classifier entry with destination `6E82` and
result `0015` for source byte `95`. Only the store-helper prefix has executed:
the classifier, destination store and helper return remain unexecuted. The
source is not a measured live `DS:6AB1` value; the earlier IF predicate and
value 11 are synthetic. Same-invocation provenance, native `345B` parity and a
complete Up route remain unverified.
