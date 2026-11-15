# Skipped SAVE operand selection

The recovered `overlay_u25_skip_1ea6.c` and `U25_SKIP_1EA6.md` show that
opcode 09 reaches the two-operand branch at 1F23 after the comparisons at
1EBD onward. The saved-script audit identifies a SAVE after IF >=; the
shared-state fixture supplies that opcode synthetically.

`pr_slot_a_skip_selection` models this bounded opcode-09 path, retaining the
final comparison flags and stopping at 1F23. The caller authenticates the
102-byte code reference, keeps guest code immutable and supplies disjoint
host objects. Unsupported inputs and reference mismatches refuse atomically.
The shared-state chain checks the selected stop, flags, refusal and prior
SAVE effect. It does not set up or execute the operand loader, establish
same-invocation provenance, complete the Up route, or prove native 345B parity.
