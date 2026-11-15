# Synthetic post-AND runner fetch

The existing `pr_slot_a_compare_runner_next` composes the runner jump,
loop gates and opcode fetch after the AND dispatcher returns. The chain test
supplies synthetic next opcode `1B` at the next script position and stops at
`31A4` after fetching it; it does not dispatch that opcode. The dispatcher
common-tail fixture, source byte `95`, earlier predicate and value 11 are
synthetic. No same-invocation source provenance, native `345B` parity or
complete Up route is established.

Evidence: `SLOT_A_COMPARE_RUNNER_NEXT.md`, `SLOT_A_SYNTHETIC_DISPATCH_RETURN.md`,
and Dunking Sheep memory session `d3959bc6-1226-4987-bc8b-3aa3b69b6a08`.
Caller-supplied code and host-object disjointness remain adapter contracts.
