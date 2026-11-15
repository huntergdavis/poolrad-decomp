# AND load-word class-four branch

The raw load-word helper in `U25_LOAD_WORD_0E0C.md` records the classifier
result and selects the class-4 path at `0E19..0EA6`. The bounded adapter
accepts classifier result 4, authenticates the supplied 142-byte reference,
checks local/code overlap, and exposes the local writes and comparison flags
at `0EA7`. Caller supplies immutable guest code and disjoint host objects.

The synthetic AND chain reaches the class-4 comparison with `C04F` in the
argument frame. It has not dereferenced `C04F`. The earlier IF predicate and
value 11 are synthetic; same-invocation provenance, native `345B` parity and
a complete Up route remain unverified.
