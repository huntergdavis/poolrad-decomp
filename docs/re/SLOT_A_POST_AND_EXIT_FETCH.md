# Post-AND EXIT fetch with synthetic state

`SLOT_A_FIRST_EXIT_9974.md` documents that the saved zero-gate first entry
fetches EXIT `00` at `9974` after AND at `996B`; its later `9975` search entry
is separate. This chain supplies `00` at `9974` as a synthetic fixture and
reuses the runner adapter through fetch at `31A4`, stopping before dispatch.
The earlier synthetic `1B`/`09` branch is an adapter exercise, not that saved
first-entry continuation.

The AND source byte `95`, dispatcher common tail, earlier predicate and value
11 are synthetic. The retained same-run AND-entry RAM hash is documented but
its local image was not found in the prior bounded search. This test establishes
no same-invocation source provenance, native EXIT transition, bytewise EXIT delta,
`345B` parity or complete Up route. Evidence: `SLOT_A_FIRST_EXIT_9974.md`,
`SLOT_A_UP_ECL_GATE.md`, `SLOT_A_SYNTHETIC_DISPATCH_RETURN.md`, and Dunking
Sheep memory session `01a13101-709d-77e1-9fcb-2a6acbff0046`. Caller-supplied
code and host-object disjointness remain adapter contracts.
