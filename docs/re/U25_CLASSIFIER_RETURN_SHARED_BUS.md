# U25 store classifier return

`pr_u25_classifier_return` executes the call-free U25 `+06FB` classifier on the shared bus. It creates the real local frame, initializes class 4, rereads the live address at each reached bound comparison, updates the class byte in source order, loads AL, and performs `RETF 2`. It checks returned CS, IP, SP, BP and SS against an explicit target; a mismatch reports the transfer after executed effects. The final comparison flags remain live.

`pr_class_return_start` and `pr_class_return_resume` extend the [store classifier entry](U25_STORE_CLASSIFIER_ENTRY_SHARED_BUS.md) through its checked near-caller return at `+0C84`. Other slot-type and earlier bounded stops remain distinct. No subsequent store-word effect is executed.

Pinned GAME.OVR SHA-256 is `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, U25 base `74D3`. The independently decoded 91-byte `[06FB,0756)` body SHA-256 is `e41b64e0284b3dff9398b1d8fe08f2c8c773f8521975021c8d10734118d39347`, matching the [store-word audit](U25_STORE_WORD_0C77.md). No original bytes are embedded.

Standalone entry requires IP `06FB`, SP in `[0004,FFF8]`, exactly 1 MiB RAM, attached EGA, and ordinary CS/DS/SS windows below `9000` or at least `B000`. The composition inherits earlier bounds and caller responsibilities, including immutable code and valid reached frames. These are bounded assumptions, not a general alias or hardware proof.

Synthetic tests cover the idle-to-Up pair route through classifier return, all range edges and adjacent values, a mismatched return, and invalid-entry atomicity. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. This composition has no native differential; subsequent store effects, other slot types, later SAVE, string execution, movement, visual parity, full gameplay and coverage credit remain unverified.
