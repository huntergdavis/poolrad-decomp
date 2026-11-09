# Next ECL dispatch after SAVE

`pr_next_dispatch_start` and `pr_next_dispatch_resume` extend [one runner continuation after SAVE](U1C_RUNNER_NEXT_SHARED_BUS.md) through the existing [ECL dispatcher prefix](U1C_ECL_DISPATCH_PREFIX.md). When the runner reaches `+2F2D`, the prefix executes on the same live bus and CPU, stopping at the actual handler entry for opcodes 0–9 or at the later-opcode boundary `+2F8C`. No second handler body is executed. Runner exit and all earlier bounded outcomes remain distinct.

The dispatcher prefix retains its source-ordered BP push, live opcode reread, source comparison flags and checked near-call frame. Its pinned U1C `[2F2D,2F8C)` span is 95 bytes, SHA-256 `3b9712886303f394eb6c8142482370ed0ebd60ab3c1b479e200d6b2708f5c246`, in GAME.OVR SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`. The linked dispatcher and runner audits supply source and frame evidence. No original bytes are embedded.

The composition inherits the earlier bounds, budgets and caller responsibilities, including immutable code outside EGA, writes avoiding code and valid reached frames. These are bounded assumptions, not a general alias or hardware proof.

Synthetic tests cover the idle-to-Up numeric SAVE route followed by a second opcode 9 to its real handler frame, and a later opcode to `+2F8C`; existing prefix tests cover all supported opcodes. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. This composition has no native differential; the next handler, further iterations, high/other classes, string execution, movement, visual parity, full gameplay and coverage credit remain unverified.
