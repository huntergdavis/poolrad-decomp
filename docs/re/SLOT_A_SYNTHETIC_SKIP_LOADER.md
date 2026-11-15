# Synthetic post-AND skip numeric loader

The existing bounded numeric loader accepts synthetic operand bytes
`00 11 02 82 6E`, loads the two operand table rows, advances the script PC
to `997B`, and returns to `1F2A`. This chain stops before the skip-helper
return. An unsupported tag refuses without CPU, RAM or status mutation.
The source byte `95`, IF opcode `1B`, skip opcode `09`, operand bytes,
dispatcher tail, earlier predicate and value 11 are synthetic. No same-run
source provenance, native `345B` parity or complete Up route is established.

Evidence: `SLOT_A_SYNTHETIC_SKIP_LOADER_ENTRY.md`, `SLOT_A_SKIP_LOADER_BODY.md`,
and Dunking Sheep memory session `997d7fe9-a031-492f-9b1a-11963b6f8cc5`.
Caller authenticates code and SYSTEM binding and keeps host objects disjoint.
