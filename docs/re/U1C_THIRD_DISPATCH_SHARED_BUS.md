# Third ECL dispatch after second SAVE

`pr_third_dispatch_start` and `pr_third_dispatch_resume` extend the [runner iteration after second SAVE](U1C_SECOND_RUNNER_NEXT_SHARED_BUS.md) through the existing [ECL dispatcher prefix](U1C_ECL_DISPATCH_PREFIX.md). The shared bus executes the live opcode checks and real call frame, stopping at an opcode 0–9 handler entry or the later-opcode boundary `+2F8C`. These outcomes are distinct; runner return and earlier second-store and other statuses pass through unchanged. No third handler body is executed.

Start rejects outer BP below 56 or any nonempty nested runner and inherited sessions before mutation. It inherits earlier frame, budget, code and caller requirements. These are bounded assumptions, not a general alias or hardware proof. No original bytes are embedded.

Synthetic integration covers idle-to-Up second low no-op and `03DE` write routes followed by a later opcode and opcode 9 respectively, plus atomic rejection and retry for each nested phase and invalid BP. Existing standalone prefix tests cover supported opcodes; runner-return and other earlier stop mappings are not separately driven here. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. No native differential, third handler body, further iterations, movement, visual parity, full gameplay or coverage credit is claimed.
