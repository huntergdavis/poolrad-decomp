# Second SAVE loader entry

`pr_second_save_start` and `pr_second_save_resume` extend the [next ECL dispatcher after SAVE](U1C_NEXT_DISPATCH_SHARED_BUS.md) only when the next handler is SAVE at `+0292`. They reuse the existing [SAVE loader-entry prefix](U1C_SAVE_LOADER_ENTRY.md) on the same live bus and CPU, building its real frame and checking the loaded stub before stopping at U25 operand-loader `+0000`. A different matched handler and a later opcode remain distinct outcomes. No second loader body or later handler effect is executed.

The wrapper rejects outer BP below 56 or a nonempty nested dispatcher or runner before mutating the session or diagnostic. Other earlier outcomes and suspended-state checks are inherited. The source evidence and pinned overlay identity are documented in the linked dispatcher and SAVE audits; no original bytes are embedded.

The composition inherits earlier bounds, budgets and caller responsibilities, including immutable code outside EGA, writes avoiding code and valid reached frames. These are bounded assumptions, not a general alias or hardware proof.

Synthetic tests cover the idle-to-Up numeric SAVE route followed by a second SAVE through its real loader frame, a different matched handler, invalid-BP and nested-session atomic rejection followed by retry, and suspended CPU tampering. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. This composition has no native differential; the second loader body, further iterations, high/other classes, string execution, movement, visual parity, full gameplay and coverage credit remain unverified.
