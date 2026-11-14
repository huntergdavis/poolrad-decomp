# Conditional first SAVE slot-to-store composition

`pr_slot_a_first_save_compose` joins the tag-`00` immediate slot-1 path at
`02B7`, ordered store-argument setup, and the fixed `11 → 6E7D` raw store on
one caller-supplied CPU and RAM image. It stops at `02CD` after checking both
helper statuses and actual CS:IP. It reports the first refused or altered
stage and retains effects already reached; it does not synthesize a return.

The caller must authenticate each fixed-size code reference and loaded binding,
provide disjoint host objects, and satisfy the constituent immutability and
alias contracts. The composition checks tag `00` and the store model checks
its exact frame; these checks do not establish provenance. See
[the slot-value boundary](SLOT_A_SAVE_SLOT_VALUE.md), [store arguments](SLOT_A_SAVE_STORE_ARGS.md),
and [fixed store](SLOT_A_FIRST_SAVE_STORE.md). The saved script audit identifies
the first SAVE as immediate `11 → 6E7D` ([evidence](SLOT_A_UP_ECL_GATE.md)).

A synthetic shared-RAM test reaches `02CD` and writes `11` to the supplied
window, checks atomic unsupported-tag refusal, and checks that an argument-code
refusal retains the reached slot result. This is conditional composition: no
same-invocation saved-game input or full native RAM comparison is established.
The SAVE epilogue, later script effects and `345B` return remain unexecuted.
