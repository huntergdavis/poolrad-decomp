# Conditional runner next-opcode fetch

`pr_slot_a_runner_opcode_fetch` models `319A..31A3`: add AX to DI with x86
arithmetic flags, read ES:`DI+6700` with 16-bit offset wrap, then store the
byte into DS:`6F7F`. It stops at `31A4`, before the dispatcher call frame.
Read-before-write order is preserved. The caller supplies full 1 MiB RAM,
disjoint host objects and independently authenticated ten-byte code; invalid
inputs and latch/code overlap refuse without changes. Reference matching does
not establish provenance.

The existing raw runner candidate `overlay_u1c_run_3171_cpu` attributes these
instructions; [the preceding prefix](SLOT_A_RUNNER_OPCODE_PREFIX.md) composes
its earlier instruction models. Synthetic tests cover full RAM/refusal,
arithmetic carry and overflow boundaries, and the shared SAVE chain fetching a
supplied next opcode `03` at `31A4`. That fixture does not prove a saved-game
same-invocation state or native full-RAM parity. The dispatcher, later script
effects and `345B` return remain unexecuted.

Build the two `poolrad_slot_a_runner` fetch test targets and run `ctest
--test-dir BUILD -R '^poolrad_slot_a_runner_(opcode_fetch|fetch_chain)_tests$'
--output-on-failure`.
