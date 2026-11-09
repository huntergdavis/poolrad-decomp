# U25 slot-value pair helper entry

`pr_u25_slot_pair_entry` executes the pair-type body at `+019E`: read each live byte local into AL while preserving AH, push both AX words, push CS, and validate the reached near call at `+01A7` to `+06AF`. It then pushes the actual return offset `+01AA` and stops at the operand-word helper entry. A malformed call stops at `+01A7` after the preceding source effects. Reads and writes use the shared bus in source order.

`pr_slot_pair_start` and `pr_slot_pair_resume` extend the [type dispatch](U25_SLOT_DISPATCH_SHARED_BUS.md) only for pair types 2 and 81. Zero, word, unmatched, SAVE-string, loader-string, budget, prompt and other stops remain distinct. No helper or later slot-value/SAVE effect is executed.

Pinned GAME.OVR SHA-256 is `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, U25 base `74D3`. The independently decoded 12-byte `[019E,01AA)` span SHA-256 is `912a995988c3c9375b44fcdce392664545a8e878b4b7b4b61d70f9af77273e12`. The [slot-value audit](U25_SLOT_VALUE_0136.md) supplies prior complete-body flat-RAM evidence. No original bytes are embedded.

Standalone entry requires IP `019E`, BP at least 18, SP=BP-10, exactly 1 MiB RAM, attached EGA, and ordinary CS/DS/SS windows below `9000` or at least `B000`. The composed entry requires outer BP at least 52 and inherits prior bounds and caller responsibilities, including immutable code outside EGA and valid reached frames. These are bounded assumptions, not a general alias or hardware proof.

Synthetic tests cover the idle-to-Up pair route and actual helper frame, preserved AH and flags, malformed opcode and displacement, and invalid-entry atomicity. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. This composition has no native differential; helper execution, later SAVE, string execution, movement, visual parity, full gameplay and coverage credit remain unverified.
