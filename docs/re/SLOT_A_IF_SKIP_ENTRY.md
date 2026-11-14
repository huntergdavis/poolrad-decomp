# IF false-branch skip entry

`U1C_IF_0B52.md` identifies the false IF >= call at 0BC1 through unit 25
stub 008E to entry 1EA6, returning to 0BC6. `U25_SKIP_1EA6.md` documents the
subsequent raw helper and its loader dependencies.

`pr_slot_a_if_skip_entry` authenticates caller-provided call and stub
references, validates their loaded far binding, builds the live far frame,
and stops at 1EA6. Unsupported inputs, mismatched bindings, or stack writes
overlapping those references refuse atomically. The caller keeps guest code
immutable and host objects disjoint. The synthetic shared-state chain checks
the entry frame and a bad stub while preserving prior effects. It does not
execute the skip helper, establish same-invocation provenance, complete the
Up route, or prove native 345B parity.
