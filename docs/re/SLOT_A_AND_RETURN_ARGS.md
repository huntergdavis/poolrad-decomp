# Synthetic AND return arguments

`U1C_AND_OR_0DEF.md` documents the second operand result store and destination
byte pushes at `0E10..0E1A`. The bounded adapter authenticates the supplied
11-byte reference, requires AX to match the explicit synthetic byte, and
preserves the source-order local store and two live descriptor reads/pushes.
It rejects code/write overlap and wrapping offsets before mutation. Caller
supplies immutable guest code and disjoint host objects.

The synthetic chain stores value `95`, pushes the destination descriptor bytes
and stops before the word-helper call at `0E1B`. The byte is not a measured
live `DS:6AB1` value; the earlier IF predicate and value 11 are synthetic.
Same-invocation provenance, native `345B` parity and a complete Up route remain
unverified.
