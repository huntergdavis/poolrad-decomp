# Synthetic COMPARE dispatcher return

The bounded U1C:2F5C continuation takes the dispatcher jump to the shared 316F epilogue, pops BP and performs RETF using the live shared-bus frame. AX and flags remain unchanged. The direct provider checks returned CS, IP, SP, BP and SS against an explicit target; a mismatch reports the reached transfer after its effects. It requires ordinary CS, DS and SS, SP=BP at most FFFA, 1 MiB RAM and attached EGA.

The composed wrapper extends [numeric COMPARE caller return](U1C_COMPARE_CALLER_RETURN_SHARED_BUS.md) through the checked runner return at U1C:31A8 using a copied target and the established frame geometry. Earlier stops and yields pass through, with inherited alias, immutable-code and nested-session constraints. Synthetic integration retains the six comparison flags through return; standalone checks cover frame and flags, stack/data alias, altered return and invalid-frame atomicity. The jump and shared epilogue were decoded read-only from ignored owner media and agree with [SAVE dispatcher-return evidence](U1C_SAVE_DISPATCH_RETURN_SHARED_BUS.md).

Prior SAVEs and operands are synthetic. Native reach, the COMPARE string branch, subsequent runner behavior, whole COMPARE readiness and credit remain unverified. No game bytes are included.
