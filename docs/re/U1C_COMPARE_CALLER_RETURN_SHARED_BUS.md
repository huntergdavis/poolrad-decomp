# Synthetic numeric COMPARE caller return

The bounded U1C:019F epilogue restores SP from BP, pops BP, and performs RETF using the live shared-bus frame. AX and flags remain unchanged. The direct provider checks returned CS, IP, SP, BP and SS against an explicit target; a mismatch reports the reached transfer after its effects. It requires ordinary CS, DS and SS, BP in [0204, FFF9], SP=BP-0204, 1 MiB RAM and attached EGA.

The composed wrapper extends [numeric helper return](U25_COMPARE_NUMERIC_RETURN_SHARED_BUS.md) through the checked dispatcher return at U1C:2F5C using a copied target and established caller-frame geometry. Earlier stops and yields pass through, with inherited alias, immutable-code and nested-session constraints. Synthetic integration retains the six numeric comparison flags through return; standalone checks cover frame and flag preservation, stack/data alias, altered return and invalid-frame atomicity. Candidate [COMPARE evidence](U1C_COMPARE_0126_CPU.md) describes the epilogue and dispatcher site.

Prior SAVEs and operands are synthetic. Native reach, the string branch, subsequent dispatcher behavior, complete COMPARE readiness and credit remain unverified. The flat-RAM numeric and load-word providers are not composed.
