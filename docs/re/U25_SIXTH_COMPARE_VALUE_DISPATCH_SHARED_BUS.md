# Sixth synthetic COMPARE first-slot dispatch

`pr_sixth_compare_value_start` and `pr_sixth_compare_value_resume` extend the [sixth COMPARE first-slot entry](U1C_SIXTH_COMPARE_SLOT_ENTRY_SHARED_BUS.md) through the existing shared-bus [slot prefix](U25_SLOT_PREFIX_SHARED_BUS.md) and [type dispatch](U25_SLOT_DISPATCH_SHARED_BUS.md). The prefix rereads the live slot index for each table access and writes its locals in source order. The dispatch preserves its final comparison flags and stops before the zero, word, pair or unmatched body. These outcomes are distinct; earlier COMPARE and other stops pass through unchanged.

Start inherits the outer BP `>=044A`, nonempty-session rejection, frame, code and alias profiles of the preceding composition. Synthetic integration covers the first slot's pair boundary and local bytes, copied target and inherited guard rejection. Existing standalone slot-dispatch tests cover all type bytes; other first-slot wrapper mappings are not separately driven here. No selected slot body executes.

The [COMPARE raw caller evidence](U1C_COMPARE_0126_CPU.md) remains candidate evidence without a readiness or caller-unlock grant. The preceding five SAVEs in this fixture are synthetic. No native saved-slot reachability, completed COMPARE, strict-credit or full-gameplay claim is made.
