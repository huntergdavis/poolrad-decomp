# Synthetic AND compare helper

`U1C_AND_OR_0DEF.md` documents the compare-word call at `0E47`; the existing
`slot_a_compare_word_effects` composition documents its U25 `18EB` and
FillChar bindings. This bounded adapter authenticates the supplied call,
stub and helper references, requires the saved second value and compare
arguments to match an explicit synthetic byte, excludes stack/DS overlap,
and exposes the raw helper status and CPU/RAM. Caller supplies immutable
guest code, keeps guest writes away from code, and supplies disjoint host
objects. Invalid bindings refuse before mutation.

The synthetic chain compares zero and `15` for source byte `95`, returning to
`0E4C`. The byte is not a measured live `DS:6AB1` value; the earlier IF
predicate and value 11 are synthetic. Same-invocation provenance, native
`345B` parity and a complete Up route remain unverified.
