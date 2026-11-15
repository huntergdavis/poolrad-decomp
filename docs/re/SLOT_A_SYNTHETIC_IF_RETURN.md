# Synthetic post-AND IF return

The existing IF-return adapter follows the caller-authenticated tail at
`0BC6` and returns to dispatcher continuation `3025`, restoring BP without
writing RAM. The chain stops before executing the dispatcher continuation.
Altered code refuses without CPU or RAM mutation. The source byte `95`, IF
opcode `1B`, skip opcode `09`, operand bytes `00 11 02 82 6E`, dispatcher
tail, earlier predicate and value 11 are synthetic. No same-run source
provenance, native `345B` parity or complete Up route is established.

Evidence: `SLOT_A_SYNTHETIC_SKIP_RETURN.md`, `SLOT_A_IF_RETURN.md`, and
Dunking Sheep memory session `01a13101-709d-77e1-9fcb-2a6acbff0046`.
Caller-supplied code and host-object disjointness remain adapter contracts.
