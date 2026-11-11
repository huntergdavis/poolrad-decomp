# Synthetic runner iteration after numeric COMPARE

The wrapper extends [COMPARE dispatcher return](U1C_COMPARE_DISPATCH_RETURN_SHARED_BUS.md) through the existing [one-iteration runner provider](U1C_RUNNER_NEXT_SHARED_BUS.md). The provider rereads live loop gates and either returns to the copied runner target or fetches the next opcode and stops at U1C:2F2D before its handler. These outcomes are distinct; earlier stops and yields pass through. Malformed reached code retains prior effects.

The wrapper inherits the prior frame, alias, immutable-code and nested-session constraints. The synthetic integration exercises a following opcode fetch and dispatcher frame while retaining the six comparison-result bytes; the pre-existing runner tests cover gate and malformed-code behavior. The earlier SAVEs, COMPARE operands and following opcode are synthetic. Native reach, the COMPARE string branch, next handler, unbounded runner behavior, full gameplay, readiness and credit remain unverified.
