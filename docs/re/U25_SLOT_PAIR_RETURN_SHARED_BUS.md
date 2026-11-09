# U25 slot-value pair helper return

`pr_pair_return_start` and `pr_pair_return_resume` extend the [slot pair helper entry](U25_SLOT_PAIR_ENTRY_SHARED_BUS.md) through the existing [shared-bus U25 operand-word leaf](U25_WORD_RETURN_SHARED_BUS.md). The leaf consumes the live pair frame and `RETF 4` is checked against the real near caller's CS, `+01AA`, BP, SS and post-cleanup SP. It stops before the slot-value local-result store. A mismatched return reports a stop after the helper's executed effects. Other dispatch and earlier bounded stops remain distinct.

The [slot-value audit](U25_SLOT_VALUE_0136.md) identifies the `+01A7` near call and its `+01AA` return; the [operand-loader audit](U25_LOADER_0000.md) identifies the complete `+06AF` leaf. Both use pinned GAME.OVR SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`. No original bytes are embedded.

The composed entry requires outer BP at least 56 so the reached helper frame satisfies its stack profile. It inherits the earlier shared-bus, immutable-code, frame, budget and caller requirements. These are bounded assumptions, not a general alias or hardware proof.

Synthetic tests cover the idle-to-Up pair route through the real return, result, flags, frame cleanup, copied target and invalid outer bound. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. This composition has no native differential; later slot-value/SAVE effects, string execution, movement, visual parity, full gameplay and coverage credit remain unverified.
