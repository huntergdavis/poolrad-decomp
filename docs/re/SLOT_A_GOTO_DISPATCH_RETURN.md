# GOTO dispatcher return and target fetch

The ignored run003 script-entry RAM identified in `SLOT_A_FIRST_EXIT_9974.md`
was rechecked at SHA-256 `f71571a2f22f6264dc39f52424a9d4d300a3429f2d1c7932cec8735a98ae0558`.
Its unit 1C `2F46` jump reaches the shared `316F` pop-BP/far-return tail.
`SLOT_A_UP_ECL_GATE.md` identifies the target `996B` as AND opcode `2F`.

`pr_slot_a_goto_dispatch_return` checks caller-authenticated jump and tail
references, then exposes the actual stack return and CPU state. It requires
immutable guest code, disjoint host objects, and a nonwrapping six-byte stack
frame. The synthetic chain continues through the existing runner to fetch a
supplied `2F` at `996B`; it does not execute AND. Its zero IF predicate and
prior value 11 are synthetic, so same-invocation provenance, native `345B`
parity, and a complete Up route remain unverified.
