# Second AND operand call

The hash-matched script-entry RAM identified in `SLOT_A_FIRST_EXIT_9974.md`
and `U1C_AND_OR_0DEF.md` establish the continuation at `0E05`: save the first
value at `BP-6`, set operand index two, and far-call the slot-value stub at
`0025`, returning to `0E10`. The adapter authenticates the supplied eleven-byte
code reference and rejects code/write overlap and wrapping frame offsets before
mutation. The caller supplies immutable code and disjoint host objects.

The synthetic chain stores `007F` and reaches the stub with slot index two.
It does not execute the stub or dereference `C04F`. The earlier IF predicate
and value 11 are synthetic. Same-invocation provenance, native `345B` parity,
and a complete Up route remain unverified.
