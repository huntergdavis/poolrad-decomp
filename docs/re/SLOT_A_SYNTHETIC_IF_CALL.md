# Synthetic post-AND IF call

The existing IF call adapter builds the return frame from call site `3021`
and reaches handler entry `0B52`, stopping before the handler. A malformed
near call stops at `3022` after the CS push; the chain test checks that
partial state. The source byte `95`, next opcode `1B`, dispatcher tail,
earlier predicate and value 11 are synthetic. No same-invocation source
provenance, native `345B` parity or complete Up route is established.

Evidence: `SLOT_A_SYNTHETIC_IF_DISPATCH.md`, `SLOT_A_IF_CALL.md`, and
Dunking Sheep memory session `01a12b3e-db20-70e1-95f9-d3fe44b8d309`.
Caller-supplied code and host-object disjointness remain adapter contracts.
