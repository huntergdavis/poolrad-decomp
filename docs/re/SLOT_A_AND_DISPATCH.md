# Slot A AND dispatch boundary

The hash-matched ignored script-entry RAM identified in
`SLOT_A_FIRST_EXIT_9974.md` selects opcode `2F` at unit 1C `30EB`, pushes CS
at `30F3`, and calls handler `0DEF` at `30F4`, returning to `30F7`.
The prior work in `U1C_AND_OR_0DEF.md` documents the handler and its
state-dependent pseudo-source; the handler is not run here.

The bounded selector accepts only opcode `2F`, independently authenticated
selector and call references, immutable guest code and disjoint host objects.
It refuses stack/code overlap and preserves the CS push if the reached call
is malformed. The synthetic chain reaches `0DEF` with its actual call frame.
The IF predicate and earlier value 11 remain synthetic; same-invocation
provenance, native `345B` parity, and a complete Up route are unverified.
