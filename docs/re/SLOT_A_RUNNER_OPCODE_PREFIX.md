# Conditional next-opcode prefix

`pr_slot_a_runner_opcode_prefix` composes the existing fixed `3582` raw
`318D`, `3190`, `3193` and `3196` instruction models. It returns the first
refused instruction offset or `319A` on success. Reached CPU/RAM effects,
including a latch store before a later refusal, remain visible. Each reused
model checks its loaded instruction, address and ZF precondition. CPU and RAM
must be disjoint; no ADD or next-opcode fetch executes here.

The reused bounded source/evidence includes
[318D](SLOT_A_SCRIPT_318D_LOAD.md) and the adjacent recovered instruction
models. A synthetic shared-RAM test composes the prior conditional SAVE chain
through `319A`, retaining the SAVE window write; an isolated test checks a
later code refusal after the latch store and an atomic first-instruction
refusal. This does not establish same-invocation saved-game provenance or a
native full-RAM comparison. The actual next opcode, subsequent handlers and
`345B` return remain unexecuted.

Build the two `poolrad_slot_a_runner_opcode` test targets and run `ctest
--test-dir BUILD -R '^poolrad_slot_a_runner_opcode_(prefix|chain)_tests$'
--output-on-failure`.
