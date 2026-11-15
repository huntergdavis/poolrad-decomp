# AND load-word classifier call

The unit 25 load-word helper documented in `U25_LOAD_WORD_0E0C.md` begins
with a six-byte local frame, copies its address argument and calls the
classifier at `06FB`, returning to `0E19`. The bounded adapter authenticates
the supplied 13-byte reference and call target, rejects stack/code overlap and
wrapping frames, and exposes the live classifier frame and arithmetic flags.
Caller supplies immutable guest code and disjoint host objects.

The synthetic AND chain reaches the classifier with `C04F` as its argument;
the classifier and source dereference have not executed. The IF predicate and
earlier value 11 are synthetic. Same-invocation provenance, native `345B`
parity and a complete Up route remain unverified.
