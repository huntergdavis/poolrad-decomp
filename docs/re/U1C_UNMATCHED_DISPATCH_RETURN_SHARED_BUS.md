# Synthetic unmatched ECL dispatcher return

The bounded U1C:316F epilogue pops BP and performs RETF using the live shared-bus frame. AX and flags remain unchanged. The direct provider checks returned CS, IP, SP, BP and SS against an explicit target; a mismatch reports the reached transfer after its effects. It requires ordinary CS, DS and SS, SP=BP at most FFFA, 1 MiB RAM and attached EGA.

The composed wrapper extends [later opcode checks](U1C_COMPARE_LATER_OPCODE_SHARED_BUS.md) only for the unmatched outcome through the checked runner return at U1C:31A8, using a copied target and established frame geometry. Matched handler stops, earlier stops and yields pass through, with inherited alias, immutable-code and nested-session constraints. Synthetic opcode 42 integration verifies the final comparison flags through return; standalone checks cover frame and flags, stack/data alias, altered return and invalid-frame atomicity. The epilogue reuses [dispatcher-return evidence](U1C_SAVE_DISPATCH_RETURN_SHARED_BUS.md).

Prior SAVEs, COMPARE operands and opcode 42 are synthetic. Native reach, matched handlers, subsequent runner behavior, whole COMPARE readiness and credit remain unverified.
