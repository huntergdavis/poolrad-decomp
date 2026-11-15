# Synthetic AND load-word return

`U25_LOAD_WORD_0E0C.md` documents the result reload and `RETF 2` at
`0F4A..0F52`. The bounded adapter authenticates the supplied nine-byte code
reference and requires its result local to match the explicit synthetic byte.
It exposes the actual saved BP and far return frame; mismatches refuse without
changes. Caller supplies immutable guest code and disjoint host objects.

The synthetic chain returns byte `95` to `0191`. An altered-frame test checks
that the actual return address is exposed. The byte is not a measured live
`DS:6AB1` value, and the earlier IF predicate and value 11 are synthetic.
Same-invocation provenance, native `345B` parity and a complete Up route remain
unverified.
