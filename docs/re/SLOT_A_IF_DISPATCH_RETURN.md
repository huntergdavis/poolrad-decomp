# IF dispatcher return

The retained loaded script-entry image identifies the three-byte jump at
3025 to the shared 316F pop-BP/far-return tail. The analogous shared tail is
described in `U1C_COMPARE_DISPATCH_RETURN_SHARED_BUS.md`.

`pr_slot_a_if_dispatch_return` authenticates caller-supplied jump and tail
references and reads the live return frame, exposing the actual BP and far
destination. Invalid inputs or wrapping stack-offset frames refuse atomically.
The caller keeps guest code immutable and host objects disjoint. The synthetic
shared-state chain checks the return to 31A8, prior effects, reference refusal
and an altered destination. It does not execute the next runner iteration,
establish same-invocation provenance, complete the Up route, or prove native
345B parity.
