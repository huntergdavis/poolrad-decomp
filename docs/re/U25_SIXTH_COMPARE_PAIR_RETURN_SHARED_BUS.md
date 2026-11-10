# Sixth synthetic COMPARE first-slot pair return

`pr_sixth_compare_pair_return_start` and `pr_sixth_compare_pair_return_resume` extend the [sixth COMPARE first-slot pair entry](U25_SIXTH_COMPARE_PAIR_ENTRY_SHARED_BUS.md) through the existing shared-bus operand-word leaf and checked `RETF 4`. The expected `+01AA` return uses the reached caller's CS, BP, SS and post-cleanup SP. Earlier slot and other outcomes pass through unchanged; a mismatched live return stops after executed effects. The slot local-result store and later COMPARE operations are not executed here.

Start inherits the outer BP `>=044A`, nonempty-session rejection, frame, code and alias profiles of the preceding composition. Synthetic integration covers the first pair's combined word, checked return and inherited frame/guard checks. Existing standalone word-return tests cover mismatched returns; other first-slot mappings are not separately driven here. This composition reuses [fifth SAVE pair return](U25_FIFTH_PAIR_RETURN_SHARED_BUS.md).

The [COMPARE raw caller evidence](U1C_COMPARE_0126_CPU.md) remains candidate evidence without a readiness or caller-unlock grant. The preceding five SAVEs and pair-type operand in this fixture are synthetic. No native saved-slot reachability or operand type, completed COMPARE, strict-credit or full-gameplay claim is made.
