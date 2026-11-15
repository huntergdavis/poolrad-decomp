# Slot A AND operand loader

The retained script-entry image and save identified in
`SLOT_A_FIRST_EXIT_9974.md` and `SLOT_A_UP_ECL_GATE.md` establish three
operands for the `996B` AND: immediate `7F`, pseudo-address `C04F`, and
destination `6E82`. The shared-state synthetic chain supplies that shape and
uses `pr_slot_a_save_loader_compose` to authenticate the live stub and loader
body and expose raw loader status and effects. The loader advances the script
PC to `9974` and returns to `0DFD` on this fixture. Callers must satisfy the
existing composition's immutable-code, authenticated-reference and disjoint
host-object requirements.

Loading the operand descriptor does not dereference `C04F`; its state-dependent
source value remains unknown. The earlier IF predicate and value 11 are
synthetic. Same-invocation provenance, native `345B` parity and a complete Up
route remain unverified.
