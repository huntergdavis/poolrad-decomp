# Runner iteration after first COMPARE

`pr_slot_a_compare_runner_next` composes the established runner jump, loop
gates, opcode prefix and opcode fetch from `31A8` through `31A4`. Its status
identifies argument refusal, jump or loop refusal, loop exit, prefix stop,
fetch refusal, or a reached fetch. Later stops preserve the state already
executed by preceding stages. The caller authenticates the supplied loop and
fetch references and maintains the component contracts, including disjoint
host objects and immutable guest code.

The saved script audit `SLOT_A_UP_ECL_GATE.md` identifies the next operation
as IF >=; `ECL_CONTRACTS.md` maps it to opcode 1B. The synthetic shared-state
chain supplies that opcode and checks its fetch while preserving the prior
SAVE and numeric comparison bytes. It does not establish same-invocation
provenance, execute the IF handler, complete the Up route, or prove native
345B parity.
