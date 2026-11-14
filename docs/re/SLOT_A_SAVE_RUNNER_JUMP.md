# Conditional SAVE return to runner loop

`pr_slot_a_save_runner_jump` adapts the shared raw CPU state to the existing
`pr_rec_u1c_31a8_jmp_loop` model. It requires the supported loaded address
`3582:31A8` and checked jump bytes, then stops at `317F` before the runner's
loop test. Invalid CPU, RAM or code refuses without changes; RAM and all CPU
fields except IP remain unchanged on success. This is a field-by-field CPU
adapter, not a general relocation model.

The reused [jump evidence and original-byte checks](SLOT_A_SCRIPT_31A8_JMP_LOOP.md)
separately document that no direct native `31A8 → 317F` capture is claimed.
A synthetic shared-state test composes [the SAVE dispatcher return](SLOT_A_SAVE_DISPATCH_RETURN.md)
through the jump, preserving the window write; it also checks a changed-code
refusal. A caller-supplied image and frames do not establish same-invocation
saved-game provenance. The loop test, subsequent script effects, native
full-RAM comparison and `345B` return remain unexecuted.

Build `poolrad_slot_a_save_runner_chain_tests`, then run `ctest --test-dir
BUILD -R '^poolrad_slot_a_save_runner_chain_tests$' --output-on-failure`.
