# Synthetic post-AND IF handler prefix

The existing IF handler-prefix adapter accepts the synthetic `1B` latch and
zero predicate, saves BP, increments the script PC and reaches skip-call
site `0BC1`. This chain stops before the skip call. Unsupported predicate
refuses without CPU or RAM mutation. The source byte `95`, next opcode `1B`,
dispatcher tail, earlier predicate and value 11 are synthetic. No same-run
source provenance, native `345B` parity or complete Up route is established.

Evidence: `SLOT_A_SYNTHETIC_IF_CALL.md`, `SLOT_A_IF_HANDLER_PREFIX.md`, and
Dunking Sheep memory session `281c8700-5acc-438b-b93d-c061e28437eb`.
Caller-supplied code and host-object disjointness remain adapter contracts.
