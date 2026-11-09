# One runner continuation after SAVE

`pr_u1c_runner_next` begins at `+31A8`, validates its jump to `+317F`, and executes one runner iteration. It rereads the live stop gates; a reached stop clears DS:442E and checks the live `RETF 2` against an explicit target. Otherwise it updates the previous-opcode latch, rereads the script PC and pointer, fetches one opcode through the shared bus, validates the reached dispatcher near call, builds its real frame and stops at `+2F2D`. Malformed code stops at the reached check with preceding effects retained. No next handler or unbounded loop is executed.

`pr_runner_next_start` and `pr_runner_next_resume` extend the [SAVE dispatcher return](U1C_SAVE_DISPATCH_RETURN_SHARED_BUS.md) through this one iteration. The runner return target is copied from the outer target with source return IP `+345B`. High addresses, other classes and earlier bounded stops remain distinct.

Pinned GAME.OVR SHA-256 is `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, U1C base `0D46`. The independently decoded 52-byte `[317F,31B3)` span SHA-256 is `2640dc9c36dc7870b0c5300763bb03da2beedffb4b2cb8818e642d69e857bfbd`. The [prior runner prefix audit](U1C_RUNNER_PREFIX_SHARED_BUS.md) supplies source and frame evidence. No original bytes are embedded.

Standalone entry requires IP `31A8`, BP in `[0004,FFF8]`, SP=BP, exactly 1 MiB RAM, attached EGA, and ordinary CS/DS/SS windows below `9000` or at least `B000`. The composed entry requires outer BP at least 56 and inherits prior bounds and caller responsibilities, including immutable code outside EGA and valid reached frames. These are bounded assumptions, not a general alias or hardware proof.

Synthetic tests cover the idle-to-Up numeric class-4 low route to the next opcode, checked stop-gate return, EGA opcode read and latches, malformed jump, copied target and invalid-entry atomicity. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. This composition has no native differential; the next handler, further iterations, high addresses, other classes, string execution, movement, visual parity, full gameplay and coverage credit remain unverified.
