# Retained-shape SAVE source at 9927

The ignored `SAVGAMA.DAT` identified in `SLOT_A_UP_ECL_GATE.md` has SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
At script PC 9927 its first operand is tag 01/address 6E7D and its second is
tag 01/address 49FD; the following retained script PC is 992E. This corrects
the separate synthetic immediate-source fixture's shorter operand shape.

`pr_slot_a_save_6e7d_slot` bounds the raw slot reader to tag 01/address 6E7D
and checks its supplied loaded references and stack/DS/window exclusions.
The class-1 hook for this address is in the quiet set documented by
`ECL_LOAD_HOOK_CLASSIFIER.md`. Caller-authenticated bindings, immutable code
and disjoint host objects remain required; unsupported profiles refuse
atomically and reached raw status/state are exposed.

The synthetic shared-state chain gives window 1 value 11 through its earlier
SAVE, reads that value at 6E7D, stores it to 49FD, and fetches a supplied SAVE opcode at PC 992E.
This synthetic value is not a measured live source. The retained save's operand
shape does not establish same-invocation provenance, a complete Up route or
native 345B parity. The subsequent SAVE dispatch is not executed.
