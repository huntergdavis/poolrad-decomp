# Unit 3B0 restricted nonextended character branch

`pr_u3b0_character_059c` continues the caller-supplied state at `+059C`,
including the nonextended stop of the merged `+0566` poll diagnostic. It models
only `[059C,05C4)`: Escape sets `[BP-63]=1`, then `[BP-60]=0`, and branches
to `+06CF`; Enter tests the live `[BP-8F]` gate, optionally writes key `0D`
and stop byte `1`, then branches to `+06CF`. All other characters stop before
`+05C4`, with the flags of the comparison against Enter. Escape does not
invent a write to the by-reference extended flag.

The caller owns the live CPU, 1 MiB A20-off RAM, prompt frame, prior effects
and stable loaded code. Source identity and prior prompt setup/draw/timer
provenance are not proved here. Entry requires IP `059C` and `BP >= 008F`;
physical byte addresses wrap at 1 MiB. The branch does not itself call the
prompt epilogue, draw or select an item, supply entry2's `provided` flag, or
prove a full prompt, native gameplay parity or strict credit. The merged
`pr_u3b0_return_06cf` may separately continue a valid `+06CF` state with its
own caller-supplied frame and target contract. The other-character path needs
its own model and reached display/selection services.

Source evidence: owner GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, unit
3B0 file base `2C8CF`, span `[059C,05C4)` 40 bytes SHA-256
`f922a3577dd06213e6b931210f393ce334a4f811765e45ee7c578df98af19245`;
the source was decoded statically, with no original game bytes included.
Existing [`U3B0_POLL_0566_DIAGNOSTIC.md`](U3B0_POLL_0566_DIAGNOSTIC.md) and
[`U3B0_RETURN_06CF_DIAGNOSTIC.md`](U3B0_RETURN_06CF_DIAGNOSTIC.md) establish
the adjacent bounded interfaces.

Synthetic tests compose FIFO → CRT → 0899 wrapper → `+059C` → `+06CF`
epilogue for Escape and enabled Enter; disabled Enter reaches the loop boundary.
They compare whole RAM and CPU for the Escape branch, assert comparison flags
and the unmodeled character boundary, and check rejected frame atomicity.
The fixtures demonstrate the models' composition, not native reachability or
visual behavior.
