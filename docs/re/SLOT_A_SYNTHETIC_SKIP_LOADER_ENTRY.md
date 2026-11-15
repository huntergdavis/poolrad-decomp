# Synthetic post-AND skip loader entry

The existing skip loader-entry adapter prepares the two-operand count and
`1F2A` return frame, then stops at unit 25 loader entry `0000` before executing
the body. A malformed near call stops after the count and CS pushes; the
chain test checks that partial state. The source byte `95`, IF opcode `1B`,
skip opcode `09`, dispatcher tail, earlier predicate and value 11 are
synthetic. No same-run source provenance, native `345B` parity or complete
Up route is established.

Evidence: `SLOT_A_SYNTHETIC_SKIP_SELECTION.md`, `SLOT_A_SKIP_LOADER_ENTRY.md`,
and Dunking Sheep memory session `997d7fe9-a031-492f-9b1a-11963b6f8cc5`.
Caller-supplied code and host-object disjointness remain adapter contracts.
