# Synthetic COMPARE numeric helper return

The bounded U25:194D epilogue pops BP and performs RETF 4 using the live shared-bus frame. It leaves AX and flags unchanged, and checks returned CS, IP, SP, BP and SS against an explicit target. A mismatched frame reports transfer after its actual effects. The direct provider requires ordinary CS, DS and SS, SP=BP with BP at most FFF9, 1 MiB RAM and attached EGA; the final stack offset may wrap.

The composed wrapper extends [numeric comparisons](U25_COMPARE_NUMERIC_FLAGS_SHARED_BUS.md) through the checked U1C:019F return using a copied target and established COMPARE frame geometry. Earlier stops and yields pass through, with inherited alias, immutable-code and nested-session constraints. Synthetic integration and standalone checks cover flag and frame preservation, stack/data alias, altered return and invalid-frame atomicity. The epilogue reuses [numeric helper evidence](OVL_0025_18EB_CPU.md).

Prior SAVEs and operands are synthetic. Native reach, the subsequent COMPARE caller epilogue, complete COMPARE, readiness and credit remain unverified. The flat-RAM numeric helper is not composed.
