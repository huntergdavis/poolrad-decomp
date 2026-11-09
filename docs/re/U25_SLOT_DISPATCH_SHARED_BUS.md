# U25 slot-value type dispatch

`pr_u25_slot_dispatch` executes the comparisons following the [slot table prefix](U25_SLOT_PREFIX_SHARED_BUS.md), preserving the last source comparison's arithmetic flags and all other registers. It stops before type-zero body `+0164`, word body `+017A` (types 1, 3, 80), pair body `+019E` (types 2, 81), or unmatched epilogue `+01AD`. The unmatched local is not initialized or read by this dispatch.

`pr_slot_dispatch_start` and `pr_slot_dispatch_resume` compose the prefix and expose these four outcomes. Earlier SAVE-string, loader-string, budget, prompt and other stops remain distinct. No selected body or later SAVE effect is executed.

Pinned GAME.OVR SHA-256 is `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, U25 base `74D3`. Independently decoded spans have SHA-256: `[0160,0164)` `fdf380073d3a467350b662190e6da1979cc42d6d526049a01ab2b824beeec56a`; `[016E,017A)` `51d447dfb97090de980563da8ab67efdddde1cb561fee266708265b3ac70e632`; `[0196,019E)` `503a2d51d4a6dea08521baecc4daad8c42934af1c0f325b3a18935176ade696e`. The [existing slot-value audit](U25_SLOT_VALUE_0136.md) supplies complete-body flat-RAM evidence. No original bytes are embedded.

Standalone entry requires IP `0160`, BP at least 10, SP=BP-10, exactly 1 MiB RAM, attached EGA, and ordinary CS/DS/SS windows below `9000` or at least `B000`. The composition inherits prior bounds and caller responsibilities. These are bounded assumptions, not a general hardware proof.

Synthetic tests cover the idle-to-Up pair-type route, all 256 type values and independently calculated final comparison flags, preserved non-arithmetic flags, and invalid-entry atomicity. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. This composition has no native differential; selected bodies, later SAVE, string execution, movement, visual parity, full gameplay and coverage credit remain unverified.
