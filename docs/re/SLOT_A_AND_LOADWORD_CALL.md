# Second AND operand load-word call

The raw slot reader in `U25_SLOT_VALUE_0136.md` pushes the assembled address
and CS at `018C..018D`, then near-calls `0E0C` at `018E`, returning to `0191`.
The bounded adapter authenticates the supplied five-byte call reference and
target, refuses stack/code overlap and wrapping frames, and exposes the live
callee frame. Caller supplies immutable guest code and disjoint host objects.

The synthetic chain reaches `0E0C` with `C04F` as its argument, but the
load-word body does not execute and its source remains undereferenced. The IF
predicate and earlier value 11 are synthetic; same-invocation provenance,
native `345B` parity and a complete Up route remain unverified.
