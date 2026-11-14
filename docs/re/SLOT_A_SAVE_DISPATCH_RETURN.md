# Conditional SAVE dispatcher return

`pr_slot_a_save_dispatch_return` models the dispatcher `2F89` jump and
`316F..3170` pop-BP/far-return tail. It reads the actual return frame and
exposes its CS:IP, preserving an altered destination. The caller supplies
full 1 MiB RAM, disjoint host objects and independently authenticated jump
and tail references. A non-wrapping six-byte stack-offset frame is required;
invalid inputs refuse without changes. Reference matching does not establish
provenance.

The retained script-entry image identified in [the first EXIT audit](SLOT_A_FIRST_EXIT_9974.md)
contains the jump to `316F` and the pop/return tail. A synthetic shared-state
test composes [the first SAVE return](SLOT_A_FIRST_SAVE_RETURN.md) into this
model and reaches a supplied `31A8` frame while retaining the window write.
The isolated test checks atomic refusal and a separately supplied return.
No same-invocation saved-game execution or native full-RAM parity is claimed;
the runner continuation, later script effects and `345B` return remain
unexecuted.

Build the two test targets and run `ctest --test-dir BUILD -R
'poolrad_slot_a_save_dispatch_(return|chain)_tests' --output-on-failure`.
