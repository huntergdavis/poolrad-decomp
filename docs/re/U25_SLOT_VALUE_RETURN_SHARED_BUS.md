# U25 pair slot-value return to SAVE

`pr_u25_slot_value_return` executes the pair continuation at `+01AA`: write AX to SS:BP-2, reread that live local into AX, restore SP and BP, and perform `RETF 2`. It compares the actual returned CS, IP, SP, BP and SS to an explicit target; a mismatch reports a transfer after the executed effects. The local write, reread and return-frame reads use the shared bus in source order.

`pr_value_return_start` and `pr_value_return_resume` extend the [pair helper return](U25_SLOT_PAIR_RETURN_SHARED_BUS.md) through the checked SAVE continuation `+02BF`. The expected SAVE frame derives from the copied outer target. Other type and earlier bounded stops remain distinct; later SAVE effects are not executed.

Pinned GAME.OVR SHA-256 is `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, U25 base `74D3`. The independently decoded 12-byte `[01AA,01B6)` span SHA-256 is `c7b61eea2a07785d174309c5fb9ac1de7e86c62f6f67e4d6a885822c9e66b9da`. The [slot-value audit](U25_SLOT_VALUE_0136.md) supplies prior complete-body flat-RAM evidence. No original bytes are embedded.

Standalone entry requires IP `01AA`, BP in `[000A,FFF8]`, SP=BP-10, exactly 1 MiB RAM, attached EGA, and ordinary CS/DS/SS windows below `9000` or at least `B000`. The composed entry requires outer BP at least 56 and inherits prior bounds and caller responsibilities, including immutable code and valid reached frames. These are bounded assumptions, not a general alias or hardware proof.

Synthetic tests cover the idle-to-Up pair path through SAVE return, result and flags, a mismatched live return after local effects, and invalid-entry atomicity. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. This composition has no native differential; other slot types, later SAVE, string execution, movement, visual parity, full gameplay and coverage credit remain unverified.
