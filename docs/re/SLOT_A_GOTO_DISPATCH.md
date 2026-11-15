# GOTO dispatch after IF-equal

The hash-matched ignored save identified in `SLOT_A_SAVE_RETAINED_SHAPE.md`
contains GOTO at 993B with tag-01 target word 996B. The retained loaded
script-entry image identifies the dispatcher PUSH CS at 2F42 and near call at
2F43 to handler 00F4; `U1C_GOTO_00F4_CPU.md` documents the handler contract.

`pr_slot_a_goto_dispatch` bounds the opcode-1 selection and call. It retains
the CS push if the reached near call is malformed, and rejects invalid input
or stack/code overlap before mutation. Callers authenticate the selector and
call references, keep code immutable and host objects disjoint.

The synthetic shared-state chain composes the IF return, dispatcher return,
runner fetch and GOTO call frame to 00F4, with a malformed-call witness. The
GOTO handler and target write are unexecuted. The IF predicate derives from a
synthetic zero, not a live measurement; same-invocation provenance, a
complete Up route and native 345B parity remain unverified.
