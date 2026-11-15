# Synthetic AND compare arguments

`U1C_AND_OR_0DEF.md` documents the zero/result pushes at `0E3E..0E46` before
the compare-word helper. The bounded adapter authenticates the supplied
nine-byte reference, requires the saved result to match `007F` AND the explicit
synthetic source byte, and checks stack/code and first-push/result overlap.
It exposes the call arguments and normalized XOR flags at `0E47`. Invalid input
refuses before mutation. Caller supplies immutable guest code and disjoint host
objects.

The synthetic chain pushes `0000` and `0015` for source byte `95`; the compare
helper has not executed. The byte is not a measured live `DS:6AB1` value, and
the earlier IF predicate and value 11 are synthetic. Same-invocation provenance,
native `345B` parity and a complete Up route remain unverified.
