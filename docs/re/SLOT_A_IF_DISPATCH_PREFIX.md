# IF >= dispatcher prefix

The saved-script audit `SLOT_A_UP_ECL_GATE.md` identifies IF >= after the
first COMPARE; `ECL_CONTRACTS.md` maps that operation to opcode 1B. The
existing `U1C_ECL_DISPATCH_PREFIX.md` and its raw provider establish that
opcodes above 09 reach 2F8C with flags from CMP AL,09.

`pr_slot_a_if_dispatch_prefix` composes the existing flat dispatcher prefix
and, for opcode 1B only, validates the caller-authenticated 78-byte selector
span and reaches 2F8C with those comparison flags. Its return is the first
reached or refused IP; frame effects already executed are retained. The caller
authenticates references, keeps guest code immutable and host objects
disjoint, and honors component contracts. The synthetic shared-state chain
checks that the earlier SAVE and numeric comparison bytes survive and that a
selector mismatch stops after the dispatcher frame. It does not execute the
later opcode checks or IF handler, establish same-invocation provenance,
complete an Up route, or prove native 345B parity.
