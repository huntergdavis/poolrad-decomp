# Synthetic post-AND skip fetch

The existing skip-helper adapter builds its frame, computes the script
pointer and fetches an explicitly synthetic opcode `09` into the latch. It
stops at `1EBD` before comparisons or the operand loader. Altered code refuses
without CPU or RAM mutation. The source byte `95`, IF opcode `1B`, skip
opcode `09`, dispatcher tail, earlier predicate and value 11 are synthetic.
No same-run source provenance, native `345B` parity or complete Up route is
established.

Evidence: `SLOT_A_SYNTHETIC_SKIP_ENTRY.md`, `SLOT_A_SKIP_FETCH.md`, and Dunking
Sheep memory session `01a12b3e-8cea-7de2-bfd1-ba3a8310754f`. Caller-supplied
code and host-object disjointness remain adapter contracts.
