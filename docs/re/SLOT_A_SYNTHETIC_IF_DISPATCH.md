# Synthetic post-AND IF dispatch

After the synthetic `1B` fetch, the existing dispatch-prefix and later-check
adapters reach the IF call site `3021`. This chain stops before the IF call
and handler. The source byte `95`, next opcode `1B`, dispatcher tail, earlier
predicate and value 11 are synthetic. No same-invocation source provenance,
native `345B` parity or complete Up route is established.

Evidence: `SLOT_A_SYNTHETIC_RUNNER_FETCH.md`, `SLOT_A_IF_LATER_DISPATCH.md`,
and Dunking Sheep memory session `01a13101-709d-77e1-9fcb-2a6acbff0046`.
Caller-supplied code and host-object disjointness remain adapter contracts.
