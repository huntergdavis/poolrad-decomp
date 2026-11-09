# U25 operand-word return on the shared bus

`pr_u25_word_return` executes the U25 `+06AF` operand-word leaf through its `RETF 4`, checking the live returned CS, IP, SP, BP and SS against an explicit target. It pushes BP, reads both argument bytes in source order, computes the word and final arithmetic flags, writes and rereads the local word, then reads the live return frame. Memory effects use the shared bus. A mismatched frame reports a transfer after the executed effects.

`pr_word_return_start` and `pr_word_return_resume` extend the [SAVE helper entry](U1C_SAVE_WORD_ENTRY_SHARED_BUS.md) to the checked SAVE continuation at `+02AD`. Earlier string, budget, prompt and other stops remain distinct; copied targets and suspended CPU checks are inherited. The leaf has no intervening call or branch; intermediate flags overwritten by the final ADD are not exposed as a stopping boundary.

The [existing original-byte loader audit](U25_LOADER_0000.md) identifies pinned GAME.OVR SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` and U25 `[06AF,06D3)` SHA-256 `53565df24104754f01dff68b10cf0dda3fe72ad394a5253c611ceb594d93e862`; that flat-RAM audit reports 16/16 leaf instructions covered. No original bytes are embedded.

Standalone entry requires IP `06AF`, SP in `[0004,FFF8]`, exactly 1 MiB RAM, EGA attached, and ordinary CS/DS/SS windows below `9000` or at least `B000`. The caller owns immutable code and valid nonwrapping reached frames. The composition inherits the earlier profiles and budgets; these bounds do not prove arbitrary hardware or alias behavior.

Synthetic tests cover the idle-to-Up two-operand path through the leaf and SAVE return, result and flags, a mismatched live return, and invalid-entry atomicity. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. This composition has no native differential. String execution, later SAVE effects, movement, visual parity, full gameplay and coverage credit remain unverified.
