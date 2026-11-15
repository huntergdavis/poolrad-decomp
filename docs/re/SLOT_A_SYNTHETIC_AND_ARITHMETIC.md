# Synthetic AND arithmetic

`U1C_AND_OR_0DEF.md` documents the destination local store and AND path at
`0E20..0E3D`. The bounded adapter authenticates the supplied 30-byte reference,
requires destination `6E82`, opcode latch `2F`, first operand `007F`, and an
explicit synthetic second byte, and checks code/write and latch/write overlap.
It exposes the destination local, low-byte result and logical flags at `0E3E`;
AF is normalized to zero. Invalid inputs refuse before mutation. Caller
supplies immutable guest code and disjoint host objects.

For synthetic byte `95`, the chain computes `15`. This is not a measured live
`DS:6AB1` value; the earlier IF predicate and value 11 are synthetic.
Same-invocation provenance, native `345B` parity and a complete Up route remain
unverified.
