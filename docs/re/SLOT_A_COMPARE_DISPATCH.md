# Conditional next COMPARE dispatch

`pr_slot_a_compare_dispatch` composes the fixed `3582:31A4` dispatcher call
frame, existing dispatcher prologue and first branch, and the opcode-`03`
comparison path to `2F58`. It stops before the COMPARE call setup. A guard
failure returns its reached offset and preserves earlier CPU/RAM effects.
Code/stack overlap at the call frame refuses before pushes. Caller-supplied
references must be independently authenticated, guest code immutable, and
host objects disjoint.

The existing [31A4 frame model](SLOT_A_SCRIPT_31A4_CALL_FRAME.md) requires ZF
set; this path cannot reuse that gate because the preceding pointer ADD writes
flags. The bounded frame here checks the same loaded call bytes without a ZF
precondition and preserves the incoming flags. The dispatcher prologue and
first branch reuse their existing conditional models; the `03` branch sequence
was decoded from the hash-pinned retained script image in
[the first EXIT audit](SLOT_A_FIRST_EXIT_9974.md).

Synthetic linked tests cover ZF-clear entry, the stack frame, later guard
refusal, unsupported opcode and the shared SAVE chain to `2F58`. They do not
establish same-invocation saved-game provenance or native full-RAM parity. The
COMPARE call and handler, later script effects, and `345B` return remain
unexecuted.

Build the two `poolrad_slot_a_compare_dispatch` test targets and run `ctest
--test-dir BUILD -R '^poolrad_slot_a_compare_dispatch(_chain)?_tests$'
--output-on-failure`.
