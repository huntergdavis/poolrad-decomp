# U25 store-word classifier entry

`pr_u25_store_classifier_entry` executes the store-word prologue at `+0C77`: push BP, allocate its two-byte local, read the live address argument at SS:BP+8, push that argument and CS, and validate the near call at `+0C81` to classifier `+06FB`. It pushes the actual return offset `+0C84` and stops at the classifier entry. SUB flags remain live. A malformed call stops at its reached instruction after preceding effects; shared-bus accesses occur in source order.

`pr_store_class_start` and `pr_store_class_resume` extend the [SAVE store-word entry](U1C_SAVE_STORE_ENTRY_SHARED_BUS.md) to this boundary. Other slot-type and earlier bounded stops remain distinct. Classification and store-word effects are not executed.

Pinned GAME.OVR SHA-256 is `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, U25 base `74D3`. The independently decoded 13-byte `[0C77,0C84)` span SHA-256 is `96791056e928dbb3a35180fa8955cb420f7c6140ca86787b75929be9a04ee526`. The [store-word audit](U25_STORE_WORD_0C77.md) supplies prior source and flat-RAM evidence. No original bytes are embedded.

Standalone entry requires IP `0C77`, SP in `[000A,FFF8]`, exactly 1 MiB RAM, attached EGA, and ordinary CS/DS/SS windows below `9000` or at least `B000`. The composition inherits earlier bounds and caller responsibilities, including immutable code outside EGA and valid reached frames. These are bounded assumptions, not a general alias or hardware proof.

Synthetic tests cover the idle-to-Up pair route through the actual classifier frame, address argument and flags, malformed opcode and displacement, and invalid-entry atomicity. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. This composition has no native differential; classification, store effects, other slot types, later SAVE, string execution, movement, visual parity, full gameplay and coverage credit remain unverified.
