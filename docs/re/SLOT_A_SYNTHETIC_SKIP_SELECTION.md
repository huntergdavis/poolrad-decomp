# Synthetic post-AND skip selection

The existing skip selection adapter checks the explicit synthetic opcode `09`,
sets comparison flags and stops at the two-operand branch `1F23` before
loader setup or execution. Altered code refuses without CPU or RAM mutation.
The source byte `95`, IF opcode `1B`, skip opcode `09`, dispatcher tail,
earlier predicate and value 11 are synthetic. No same-run source provenance,
native `345B` parity or complete Up route is established.

Evidence: `SLOT_A_SYNTHETIC_SKIP_FETCH.md`, `SLOT_A_SKIP_SELECTION.md`, and
Dunking Sheep memory session `01a13101-709d-77e1-9fcb-2a6acbff0046`.
Caller-supplied code and host-object disjointness remain adapter contracts.
