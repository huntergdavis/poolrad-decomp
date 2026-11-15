# Synthetic post-AND IF dispatcher return

The existing IF dispatcher-return adapter follows caller-authenticated jump
and common-tail references from `3025` to runner `31A8`, restoring BP without
writing RAM. The chain stops before the next runner iteration. Altered jump
code refuses without CPU or RAM mutation. The common dispatcher tail is a
synthetic fixture; source byte `95`, IF opcode `1B`, skip opcode `09`, operand
bytes `00 11 02 82 6E`, earlier predicate and value 11 are synthetic. No
same-run source provenance, native `345B` parity or complete Up route is
established.

Evidence: `SLOT_A_SYNTHETIC_IF_RETURN.md`, `SLOT_A_IF_DISPATCH_RETURN.md`,
and Dunking Sheep memory session `01a12b3e-db20-70e1-95f9-d3fe44b8d309`.
Caller-supplied code and host-object disjointness remain adapter contracts.
