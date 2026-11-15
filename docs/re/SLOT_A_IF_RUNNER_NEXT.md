# Runner iteration after IF skip

`SLOT_A_UP_ECL_GATE.md` identifies a SAVE at script PC 9927 after the
false IF >= skips the preceding statement. The synthetic shared-state chain
supplies that next SAVE opcode and reuses `pr_slot_a_compare_runner_next` for
the subsequent runner iteration from 31A8 to opcode fetch at 31A4.

The fixture checks the opcode latch, preserved script PC and earlier SAVE and
comparison effects, plus jump refusal. The reused adapter retains its
component-level refusal and loop-exit behavior. This conditional composition
does not execute the next SAVE dispatch, establish same-invocation
provenance, complete the Up route, or prove native 345B parity.
