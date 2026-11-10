# Sixth synthetic COMPARE first-slot pair entry

`pr_sixth_compare_pair_start` and `pr_sixth_compare_pair_resume` extend the [sixth COMPARE first-slot dispatch](U25_SIXTH_COMPARE_VALUE_DISPATCH_SHARED_BUS.md) through the existing shared-bus [pair helper entry](U25_SLOT_PAIR_ENTRY_SHARED_BUS.md) only for the pair outcome. It rereads the two live local bytes in source order, pushes the arguments and actual return frame, validates the near call, and stops at the operand-word helper entry `+06AF`. Other slot and earlier outcomes pass through unchanged. A malformed reached call stops after preceding effects; the helper body does not execute.

Start inherits the outer BP `>=044A`, nonempty-session rejection, frame, code and alias profiles of the preceding composition. Synthetic integration covers the first slot's pair entry and frame, copied target and inherited guard rejection. Existing standalone pair-entry tests cover malformed calls and aliases; other first-slot mappings are not separately driven here.

The [COMPARE raw caller evidence](U1C_COMPARE_0126_CPU.md) remains candidate evidence without a readiness or caller-unlock grant. The preceding five SAVEs and pair-type operand in this fixture are synthetic. No native saved-slot reachability or operand type, completed COMPARE, strict-credit or full-gameplay claim is made.
