# Synthetic later ECL opcode checks

The bounded U1C:2F8C continuation executes the remaining opcode comparisons in original order. It stops before the reached PUSH CS for a matching later opcode, or at the shared 316F epilogue when none matches. AL and all non-arithmetic state remain unchanged; the last CMP flags remain live. The direct entry requires ordinary CS, DS and SS, SP=BP at most FFFA, 1 MiB RAM and attached EGA. It makes no memory writes and assumes immutable valid code.

The wrapper extends [post-COMPARE dispatcher prefix](U1C_COMPARE_NEXT_DISPATCH_SHARED_BUS.md), preserving earlier stops and the inherited alias, copied-target and nested-session constraints. The synthetic opcode 42 reaches the unmatched epilogue; standalone tests cover all 256 opcode values, decoded stop addresses and independently calculated final flags, plus invalid-frame atomicity. Instruction order and boundaries were decoded read-only from ignored owner media; [dispatcher prefix evidence](U1C_ECL_DISPATCH_PREFIX.md) identifies the full original span. No game bytes are included.

Prior SAVEs, COMPARE operands and opcode 42 are synthetic. Native reach, matched handler calls and bodies, epilogue return, COMPARE string behavior, readiness and credit remain unverified.
