# Second AND operand tag test

`U25_SLOT_VALUE_0136.md` describes the raw slot reader's tag dispatch.
The hash-matched script-entry image in `SLOT_A_FIRST_EXIT_9974.md` confirms
that tag `01` at `0156` selects the address path at `017A`. This bounded
adapter authenticates the supplied code reference, rereads and sign-extends
the live stack index, reads the tag, and exposes the comparison flags and
branch state. Unsupported tags refuse without changes. Caller supplies
immutable guest code and disjoint host objects.

The synthetic chain reaches `017A` with tag `01`. It has not assembled or
dereferenced `C04F`. The IF predicate and earlier value 11 are synthetic;
same-invocation provenance, native `345B` parity and a complete Up route remain
unverified.
