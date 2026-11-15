# Second AND operand address assembly

The raw slot reader in `U25_SLOT_VALUE_0136.md` assembles the two saved
operand bytes at `017A..018B`, ending with a word ADD. The bounded adapter
authenticates the supplied 18-byte reference, reads the live stack locals,
and exposes AX, DX, CL and the ADD flags at `018C`. Caller supplies immutable
guest code and disjoint host objects; invalid inputs refuse without changes.

The synthetic chain assembles `C04F` but stops before the push and load-word
call. It does not dereference `C04F`. The IF predicate and earlier value 11
are synthetic; same-invocation provenance, native `345B` parity and a complete
Up route remain unverified.
