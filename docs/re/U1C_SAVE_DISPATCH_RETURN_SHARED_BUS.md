# SAVE dispatcher return to runner

`pr_u1c_save_dispatch_return` continues the SAVE dispatcher path at `+2F89`, takes its jump to `+316F`, reads the live saved BP and far return frame, and performs `RETF`. It checks returned CS, IP, SP, BP and SS against an explicit target; a mismatch reports the transfer after executed effects. Shared-bus reads occur in source order and arithmetic flags are preserved.

`pr_dispatch_return_start` and `pr_dispatch_return_resume` extend the completed [numeric SAVE handler return](U1C_SAVE_HANDLER_RETURN_SHARED_BUS.md) through the checked runner continuation `+31A8`. The expected runner frame derives from the copied outer target. High addresses, other classes and earlier bounded stops remain distinct; the runner loop jump is not executed.

Pinned GAME.OVR SHA-256 is `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, U1C base `0D46`. The independently decoded `[2F89,2F8C)` jump SHA-256 is `f02c827efce940273890467487ee46faaad548fe64df8f6eb636f0823b94ec64`; `[316F,3171)` epilogue SHA-256 is `05303d46935f1d6bc7f492470521a209a51050b261d452a788f042866b5d72cd`. The [dispatcher prefix](U1C_ECL_DISPATCH_PREFIX.md) and [runner audit](U1C_RUNNER_PREFIX_SHARED_BUS.md) supply prior source evidence. No original bytes are embedded.

Standalone entry requires IP `2F89`, SP=BP at most `FFFA`, exactly 1 MiB RAM, attached EGA, and ordinary CS/DS/SS windows below `9000` or at least `B000`. The composed entry requires outer BP at least 56 and inherits prior bounds and caller responsibilities, including immutable code and valid reached frames. These are bounded assumptions, not a general alias or hardware proof.

Synthetic tests cover the idle-to-Up numeric class-4 low route through runner return, preserved flags, a mismatched live return, copied target and invalid-entry atomicity. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. This composition has no native differential; runner continuation, high addresses, other classes, string execution, movement, visual parity, full gameplay and coverage credit remain unverified.
