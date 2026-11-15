# Synthetic AND store arguments

`U1C_AND_OR_0DEF.md` documents the destination/result pushes at `0E4C..0E54`
before the store-word helper. The bounded adapter authenticates the supplied
nine-byte reference, requires destination `6E82` and a result matching `007F`
AND the explicit synthetic source byte, and checks stack/code and first-push/
result overlap. It exposes the call arguments and normalized XOR flags at
`0E55`. Invalid input refuses before mutation. Caller supplies immutable guest
code and disjoint host objects.

The synthetic chain pushes destination `6E82` and result `0015` for source byte
`95`; the store helper has not executed. The byte is not a measured live
`DS:6AB1` value, and the earlier IF predicate and value 11 are synthetic.
Same-invocation provenance, native `345B` parity and a complete Up route remain
unverified.
