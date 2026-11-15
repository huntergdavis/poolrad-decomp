# SAVE dispatch after IF skip

`SLOT_A_UP_ECL_GATE.md` identifies a SAVE at script PC 9927 after the false
IF >= path. The synthetic shared-state fixture supplies that opcode and
reuses the existing dispatcher prefix, SAVE selector and call-frame adapters
to reach handler 0292. Their caller-authenticated references, immutable-code,
disjoint-host and stack assumptions remain in force.

The chain checks the reached handler frame, preserved earlier effects and
selection-reference refusal. It does not execute this SAVE handler, establish
same-invocation provenance, complete the Up route, or prove native 345B parity.
