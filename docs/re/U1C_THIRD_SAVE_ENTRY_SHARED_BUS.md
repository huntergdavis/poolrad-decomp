# Third SAVE loader entry

`pr_third_save_start` and `pr_third_save_resume` extend the [third ECL dispatch](U1C_THIRD_DISPATCH_SHARED_BUS.md) only when it reaches SAVE handler `+0292`. The existing [SAVE loader-entry prefix](U1C_SAVE_LOADER_ENTRY.md) executes on the shared bus, constructs the live loader frame and checks the stub before stopping at U25 loader `+0000`. Different matched handlers and later opcodes remain distinct, as do earlier outcomes. No third loader body or later SAVE effect is executed.

Start rejects outer BP below 56 or any nonempty nested dispatcher and inherited sessions before mutation. It inherits earlier frame, budget, code and caller requirements. These are bounded assumptions, not a general alias or hardware proof. No original bytes are embedded.

Synthetic integration covers the idle-to-Up second SAVE route followed by a third SAVE loader frame, a different matched handler, a later opcode, and atomic rejection and retry for each nested phase and invalid BP. Existing standalone tests cover malformed calls and stubs; other earlier stop mappings are not separately driven here. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. No native differential, third loader body, later SAVE, movement, visual parity, full gameplay or coverage credit is claimed.
