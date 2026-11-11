# Synthetic IF-equal entry after numeric COMPARE

The bounded U1C:3021 IF-family call site pushes CS, checks the near call at 3022 to 0B52, pushes return 3025 and stops before the IF body. It preserves flags and uses the shared bus for the live call frame. A malformed opcode or displacement stops after the CS push. The direct entry requires ordinary CS, DS and SS, SP=BP at least 4, exactly 1 MiB RAM and attached EGA, with immutable valid code.

The wrapper extends [later opcode checks](U1C_COMPARE_LATER_OPCODE_SHARED_BUS.md) only when they reach the IF-family call site, passing other matched, unmatched and earlier outcomes through. It inherits frame, alias and nested-session constraints. Synthetic integration follows numeric COMPARE with opcode 16 and retains the six result bytes; standalone tests cover the call frame, flags, stack/data alias, malformed opcode and displacement, and invalid-frame atomicity.

[Slot A static gate evidence](SLOT_A_UP_ECL_GATE.md) places IF-equal after the COMPARE, and [ECL contracts](ECL_CONTRACTS.md) identify opcode 16 as IF-equal. The earlier SAVEs and operands and this IF opcode are synthetic; their ordering does not establish native reach. The IF body, subsequent runner behavior, whole COMPARE readiness and credit remain unverified.
