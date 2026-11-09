# Numeric SAVE handler return

`pr_u1c_save_handler_return` continues the numeric SAVE path at `+02CD`, takes its jump to `+02DC`, restores SP from BP, reads the live saved BP and far return frame, and performs `RETF`. It checks returned CS, IP, SP, BP and SS against an explicit target; a mismatch reports the transfer after executed effects. Shared-bus reads occur in source order and arithmetic flags are preserved.

`pr_save_handler_start` and `pr_save_handler_resume` extend the completed [class-4 low store return](U25_STORE_RETURN_SHARED_BUS.md) through the checked dispatcher continuation `+2F89`. The expected dispatcher frame derives from the copied outer target. High addresses, other classes and earlier bounded stops remain distinct; dispatcher continuation is not executed.

Pinned GAME.OVR SHA-256 is `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, U1C base `0D46`. The independently decoded `[02CD,02CF)` jump SHA-256 is `06c8c2f66b59d22018450ce88a0d5e955fcc74c7e8b2a13467889a08937f39c6`; `[02DC,02E0)` epilogue SHA-256 is `d938592002da1e8f84eae35df938d5b693d827276f6e045d142340640128831a`. The [SAVE audit](CANDIDATE_SCOUT_DS41B_BATCH1.md) and [dispatcher prefix](U1C_ECL_DISPATCH_PREFIX.md) supply prior source evidence. No original bytes are embedded.

Standalone entry requires IP `02CD`, BP in `[0004,FFFA]`, SP=BP-4, exactly 1 MiB RAM, attached EGA, and ordinary CS/DS/SS windows below `9000` or at least `B000`. The composed entry requires outer BP at least 56 and inherits prior bounds and caller responsibilities, including immutable code and valid reached frames. These are bounded assumptions, not a general alias or hardware proof.

Synthetic tests cover the idle-to-Up numeric class-4 low route through the dispatcher return, preserved flags, a mismatched live return, copied target and invalid-entry atomicity. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. This composition has no native differential; high addresses, other class bodies, dispatcher continuation, string execution, movement, visual parity, full gameplay and coverage credit remain unverified.
