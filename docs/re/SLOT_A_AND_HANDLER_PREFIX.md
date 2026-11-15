# Slot A AND handler prefix

The hash-matched script-entry RAM in `SLOT_A_FIRST_EXIT_9974.md` and the
handler audit `U1C_AND_OR_0DEF.md` establish `0DEF..0DF7`: save BP, reserve
an eight-byte local frame, load operand count three, and push AX. The bounded
adapter checks the caller-authenticated nine-byte code reference, rejects
stack/code overlap and a wrapping stack frame, and stops before the loader
call at `0DF8`. Caller supplies immutable guest code and disjoint host objects.

The synthetic shared-state chain reaches that stop with the actual count and
frame. It does not load operands or read the state-dependent `C04F` source.
The IF predicate and earlier value 11 remain synthetic; same-invocation
provenance, native `345B` parity and a complete Up route remain unverified.
