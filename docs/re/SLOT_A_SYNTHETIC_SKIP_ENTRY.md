# Synthetic post-AND IF skip entry

The existing false-branch skip-entry adapter authenticates the supplied call
and stub, prepares the `0BC6` far return frame and stops at unit 25 entry
`1EA6` before executing the helper. A mismatched stub refuses without CPU or
RAM mutation. The source byte `95`, opcode `1B`, dispatcher tail, earlier
predicate and value 11 are synthetic. No same-run source provenance, native
`345B` parity or complete Up route is established.

Evidence: `SLOT_A_SYNTHETIC_IF_PREFIX.md`, `SLOT_A_IF_SKIP_ENTRY.md`, and
Dunking Sheep memory session `281c8700-5acc-438b-b93d-c061e28437eb`.
Caller-supplied code and host-object disjointness remain adapter contracts.
