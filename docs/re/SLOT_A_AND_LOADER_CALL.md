# AND operand-loader call

The retained script-entry image identified in `SLOT_A_FIRST_EXIT_9974.md`
and the handler audit `U1C_AND_OR_0DEF.md` establish the far call at unit 1C
`0DF8` to the operand-loader stub `002A`, returning to `0DFD`. The bounded
adapter authenticates the caller-supplied five-byte call reference, reads its
segment, rejects call-frame/code overlap, and exposes the actual far frame.
It requires immutable guest code and disjoint host objects.

The synthetic chain reaches the stub with count three on the stack. The
loader and its operands remain unexecuted, so no `C04F` source value is
claimed. The earlier IF predicate and value 11 are synthetic; same-invocation
provenance, native `345B` parity and a complete Up route remain unverified.
