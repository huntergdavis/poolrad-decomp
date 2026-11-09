# U25 store-word class-4 low addresses

`pr_u25_store_class4_low` begins at `+0D14`. It compares the live address argument with `BF68`; high addresses stop at `+0D5B` before their body. For lower addresses it rereads the address into AX and follows the source comparisons. Addresses `03DE`, `00B8` and `00B9` read the live value argument and write its word to DS:84CC, 84CE or 84D0 respectively. Addresses `00FB`, `00FC`, `00B1`, and other low values follow their source no-op paths. The low path stops at the common epilogue `+0E06` with the final comparison flags live. Shared-bus reads and writes occur in source order.

`pr_class4_low_start` and `pr_class4_low_resume` extend the [store class dispatch](U25_STORE_CLASS_DISPATCH_SHARED_BUS.md) for class 4. Other classes, unmatched values and earlier bounded stops remain distinct. The high-address body and common epilogue are not executed.

Pinned GAME.OVR SHA-256 is `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, U25 base `74D3`. The independently decoded 71-byte `[0D14,0D5B)` span SHA-256 is `c1c4fc24946609197de8091bf867af567565494a208ab611164999f7170f99c9`. The [store-word audit](U25_STORE_WORD_0C77.md) supplies prior source and flat-RAM evidence. No original bytes are embedded.

Standalone entry requires IP `0D14`, BP in `[0002,FFF6]`, SP=BP-2, exactly 1 MiB RAM, attached EGA, and ordinary CS/DS/SS windows below `9000` or at least `B000`. The composition inherits earlier bounds and caller responsibilities. These are bounded assumptions, not a general alias or hardware proof.

Synthetic tests cover the idle-to-Up class-4 low route, the three full word stores, explicit no-op cases, other low values, the high threshold, final flags, preserved non-arithmetic flags and invalid-entry atomicity. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. This composition has no native differential; high addresses, other class bodies, common return, later SAVE, string execution, movement, visual parity, full gameplay and coverage credit remain unverified.
