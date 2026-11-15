# Synthetic post-AND skip helper return

The existing skip-return adapter follows the caller-authenticated jump and
tail from `1F2A` and returns to IF continuation `0BC6`, restoring BP without
writing RAM. The chain stops before executing the IF return. Altered code
refuses without CPU or RAM mutation. The source byte `95`, IF opcode `1B`,
skip opcode `09`, operand bytes `00 11 02 82 6E`, dispatcher tail, earlier
predicate and value 11 are synthetic. No same-run source provenance, native
`345B` parity or complete Up route is established.

Evidence: `SLOT_A_SYNTHETIC_SKIP_LOADER.md`, `SLOT_A_SKIP_RETURN.md`, and
Dunking Sheep memory session `01a12b3e-8cea-7de2-bfd1-ba3a8310754f`.
Caller-supplied code and host-object disjointness remain adapter contracts.
