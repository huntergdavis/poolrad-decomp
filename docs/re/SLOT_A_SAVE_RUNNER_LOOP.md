# Conditional SAVE runner loop test

`pr_slot_a_save_runner_loop` models `317F..318C`, testing DS:`442E` and,
only if clear, DS:`49FF`, then stopping before exit at `31AA` or the next
opcode load at `318D`. Arithmetic flags reflect the last reached comparison.
The caller supplies full 1 MiB RAM, disjoint host objects, and independently
authenticated fourteen-byte code; invalid inputs refuse without changes.
Reference matching does not establish provenance.

The existing raw runner candidate `overlay_u1c_run_3171_cpu` attributes these
instructions; [the preceding jump](SLOT_A_SAVE_RUNNER_JUMP.md) uses the existing
fixed loaded model. Synthetic tests check each stop flag across all 256 byte
values, short-circuiting, flags, RAM/refusal and the shared SAVE chain to
`318D`. The next opcode, subsequent script effects, same-invocation native
binding, full-RAM comparison and `345B` return remain unproved.

Build the two `poolrad_slot_a_save_runner_loop` test targets and run `ctest
--test-dir BUILD -R '^poolrad_slot_a_save_runner_loop(_chain)?_tests$'
--output-on-failure`.
