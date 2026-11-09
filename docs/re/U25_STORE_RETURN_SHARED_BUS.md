# U25 class-4 low store return to SAVE

`pr_u25_store_return` executes the common store-word epilogue at `+0E06`: restore SP from BP, read the live saved BP and far return frame, and perform `RETF 4`. It checks returned CS, IP, SP, BP and SS against an explicit target; a mismatch reports the transfer after executed effects. Reads use the shared bus in source order and arithmetic flags are preserved.

`pr_store_return_start` and `pr_store_return_resume` extend the completed [class-4 low path](U25_STORE_CLASS4_LOW_SHARED_BUS.md) through its checked SAVE continuation `+02CD`. The expected SAVE frame derives from the copied outer target. High addresses, other classes and earlier bounded stops remain distinct; subsequent SAVE effects are not executed.

Pinned GAME.OVR SHA-256 is `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, U25 base `74D3`. The independently decoded six-byte `[0E06,0E0C)` span SHA-256 is `35aa075706a5bcba3d89e0950c3aba91ad5a7c35573a4f4e0a2949e36b70cc6b`, consistent with the [store-word audit](U25_STORE_WORD_0C77.md). No original bytes are embedded.

Standalone entry requires IP `0E06`, BP in `[0002,FFF6]`, SP=BP-2, exactly 1 MiB RAM, attached EGA, and ordinary CS/DS/SS windows below `9000` or at least `B000`. The composed entry requires outer BP at least 56 and inherits prior bounds and caller responsibilities. These are bounded assumptions, not a general alias or hardware proof.

Synthetic tests cover the idle-to-Up class-4 low route through SAVE return, preserved flags, a mismatched live return, copied target and invalid-entry atomicity. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. This composition has no native differential; high addresses, other class bodies, later SAVE, string execution, movement, visual parity, full gameplay and coverage credit remain unverified.
