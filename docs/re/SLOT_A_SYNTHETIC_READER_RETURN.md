# Synthetic AND slot-reader return

`U25_SLOT_VALUE_0136.md` documents the address-path result store and reader
`RETF 2` at `0191..01B5`. The bounded adapter authenticates the supplied
37-byte reference, requires AX to match the explicit synthetic byte, checks
local/code overlap and exposes the actual saved BP and far return frame.
Mismatches refuse before mutation. Caller supplies immutable guest code and
disjoint host objects.

The synthetic chain stores and returns `95` to `0E10`. An altered-frame test
checks that the actual return address is exposed. The byte is not a measured
live `DS:6AB1` value; the earlier IF predicate and value 11 are synthetic.
Same-invocation provenance, native `345B` parity and a complete Up route remain
unverified.
