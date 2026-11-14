# Skip-helper opcode fetch

The `U25_SKIP_1EA6.md` contract and `overlay_u25_skip_1ea6.c` identify the
entry prologue, script pointer arithmetic, opcode fetch and latch update
through 1EBA. The saved-script audit identifies a SAVE following IF >=; the
shared-state fixture supplies its opcode synthetically.

`pr_slot_a_skip_fetch` stops at 1EBD before the opcode comparisons. It retains
the actual frame, pointer arithmetic flags, register values and latch write.
Caller-authenticated code, immutable guest code and disjoint host objects are
required; stack/DS or stack/source aliases refuse atomically. The synthetic
chain checks opcode 09, the arithmetic state and prior SAVE effect. It does
not execute operand-count selection or the loader, establish same-invocation
provenance, complete the Up route, or prove native 345B parity.
