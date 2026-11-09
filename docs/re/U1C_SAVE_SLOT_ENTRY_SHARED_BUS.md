# SAVE numeric slot-value entry

`pr_u1c_save_slot_entry` executes the numeric SAVE continuation at `+02B7`: preserve AH while setting AL to 1, push AX, validate the reached far call at `+02BA` to stub `+0025`, build its live return frame to `+02BF`, and validate the stub's far jump to U25 `+0136`. It reads the live target segment and stops at the slot-value helper entry. Malformed calls or stubs and nonordinary targets stop at the reached frame after prior source effects.

`pr_save_slot_start` and `pr_save_slot_resume` extend the [SAVE tag branch](U1C_SAVE_TAG_BRANCH_SHARED_BUS.md) on the numeric path. The SAVE string branch and the earlier loader-string, budget, prompt and other stops remain distinct. No helper effects or later SAVE writes are fabricated.

The [pinned SAVE disassembly](CANDIDATE_SCOUT_DS41B_BATCH1.md) supplies the call sequence and the [U25 slot-value audit](U25_SLOT_VALUE_0136.md) identifies the helper. Pinned GAME.OVR SHA-256 is `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`; SAVE `[0292,02E0)` SHA-256 is `7f66f8be811fe835d3ed3ab0fc0772f57e10ab39b5b5beda8aa2542262f92ebb`. No original bytes are embedded.

Standalone entry requires IP `02B7`, SP at least 6, exactly 1 MiB RAM, attached EGA, and ordinary CS/DS/SS windows below `9000` or at least `B000`. The caller owns immutable code outside EGA, writes avoiding code, live binding and valid reached frames. The composition inherits prior profiles and budgets; these bounds do not prove arbitrary aliases or hardware behavior.

Synthetic tests cover the idle-to-Up numeric path through the slot-value frame, AH and flags, malformed call and stub, and invalid-entry atomicity. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. This composition has no native differential; slot-value execution, string execution, later SAVE effects, movement, visual parity, full gameplay and coverage credit remain unverified.
