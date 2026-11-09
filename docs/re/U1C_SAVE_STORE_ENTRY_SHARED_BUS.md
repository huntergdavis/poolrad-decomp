# SAVE store-word helper entry

`pr_u1c_save_store_entry` executes numeric SAVE `+02BF..+02CD`: store AX in the live SS:BP-2 local, read and push the destination at BP-4, reread and push the value at BP-2, and validate the reached far call to stub `+0052`. It constructs the actual return frame to `+02CD`, checks the stub's far jump to U25 `+0C77`, reads the live target segment and stops at the store-word helper entry. Shared-bus reads and writes occur in source order. Malformed call or stub and nonordinary target stop at the reached frame after prior effects.

`pr_save_store_start` and `pr_save_store_resume` extend the [pair slot-value return](U25_SLOT_VALUE_RETURN_SHARED_BUS.md) on its completed path. Other slot types and earlier bounded stops remain distinct. No store-word or later SAVE effect is executed.

Pinned GAME.OVR SHA-256 is `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, U1C base `0D46`. The independently decoded 14-byte `[02BF,02CD)` span SHA-256 is `0fdac4ea88e3f5971f228e622c0db42e0cf910cdf733c662ab32594805e7df0a`. The [SAVE audit](CANDIDATE_SCOUT_DS41B_BATCH1.md) supplies the call sequence and the [store-word audit](U25_STORE_WORD_0C77.md) identifies the helper. No original bytes are embedded.

Standalone entry requires IP `02BF`, BP at least 12, SP=BP-4, exactly 1 MiB RAM, attached EGA, and ordinary CS/DS/SS windows below `9000` or at least `B000`. Caller owns immutable code outside EGA, writes avoiding code, live bindings and valid reached frames. The composition inherits earlier bounds and caller responsibilities; these are not a general alias or hardware proof.

Synthetic tests cover the idle-to-Up pair route through the real store-word frame, argument order and flags, malformed call and stub, and invalid-entry atomicity. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. This composition has no native differential; store-word execution, other slot types, later SAVE, string execution, movement, visual parity, full gameplay and coverage credit remain unverified.
