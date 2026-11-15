# Second AND operand descriptor bytes

The unit 25 reader documented in `U25_SLOT_VALUE_0136.md` reads the low and
high descriptor bytes at `013C..0155`, reloading and sign-extending the live
stack index for each read and saving the bytes into separate stack locals.
The bounded adapter authenticates the supplied 26-byte reference, preserves
read/write order, rejects code/write overlap, and stops at `0156`. Caller
supplies immutable guest code and disjoint host objects.

The synthetic chain reads descriptor bytes `4F` and `C0` for slot two. The
reader has not tested the tag or dereferenced `C04F`. The IF predicate and
earlier value 11 are synthetic; same-invocation provenance, native `345B`
parity and a complete Up route remain unverified.
