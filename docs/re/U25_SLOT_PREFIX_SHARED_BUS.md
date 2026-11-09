# U25 slot-value table prefix

`pr_u25_slot_prefix` executes `+0136..+0160`: create the real ten-byte local frame, then independently reread and sign-extend the slot index for each of the DS:7085, DS:7045 and DS:7005 table reads. It writes the first two bytes to their live SS locals in source order and stops before the type comparison at `+0160`. Each read and write uses the shared bus, so earlier local writes can affect later table reads. SUB flags remain live at the stop.

`pr_slot_prefix_start` and `pr_slot_prefix_resume` extend the [numeric SAVE helper entry](U1C_SAVE_SLOT_ENTRY_SHARED_BUS.md) to that boundary. Earlier SAVE-string, loader-string, budget, prompt and other stops remain distinct. Type dispatch and helper effects are not executed.

Pinned GAME.OVR SHA-256 is `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, U25 base `74D3`. The 42-byte `[0136,0160)` prefix SHA-256 is `8db7f792d5d79e199149b73bfc3fb8cc92fcebc2f9b171b744c383f7dd4e4b98`; the prefix was independently decoded from that overlay. The [existing slot-value audit](U25_SLOT_VALUE_0136.md) identifies the complete body and prior flat-RAM evidence. No original bytes are embedded.

Standalone entry requires IP `0136`, SP in `[000C,FFFA]`, exactly 1 MiB RAM, attached EGA, and ordinary CS/DS/SS windows below `9000` or at least `B000`. The composed entry requires outer BP at least 44 and inherits prior bounds and caller responsibilities. Caller owns immutable code and valid reached frames. These are bounded assumptions, not a general alias or hardware proof.

Synthetic tests cover the idle-to-Up numeric route, signed `FF` index, a local/table alias, and invalid-entry atomicity. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. This composition has no native differential; type dispatch, later SAVE, string execution, movement, visual parity, full gameplay and coverage credit remain unverified.
