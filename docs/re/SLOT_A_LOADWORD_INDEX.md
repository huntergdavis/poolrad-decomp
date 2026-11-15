# AND load-word class-four index

The raw load-word helper documented in `U25_LOAD_WORD_0E0C.md` selects the
high class-four table for `C04F`, subtracts `C04B`, writes index 4 back to its
argument, and reaches the source read at `0F3B`. The bounded adapter accepts
only the `C04F` input, authenticates the supplied 148-byte reference, rejects
argument/code overlap, and exposes AX, flags and the argument write. Caller
supplies immutable guest code and disjoint host objects.

The synthetic chain stops before reading `DS:6AB1`; its value is unverified.
The earlier IF predicate and value 11 are synthetic; same-invocation
provenance, native `345B` parity and a complete Up route remain unverified.
