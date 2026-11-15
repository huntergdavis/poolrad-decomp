# First AND operand value

`U1C_AND_OR_0DEF.md` and `SLOT_A_UP_ECL_GATE.md` identify the first AND
operand as immediate `7F` and its unit 25 slot-value call at `0DFD..0E04`.
The bounded adapter reuses the immediate-slot composition, authenticates the
caller-supplied call, stub and helper references, and accepts only tag 00 and
value `7F`. It exposes the raw helper status and resulting CPU and RAM. Caller
supplies immutable guest code and disjoint host objects; unsupported inputs
refuse before mutation.

The synthetic chain returns `007F` in AX at `0E05`; it does not evaluate the
second operand `C04F`. The earlier IF predicate and value 11 are synthetic.
Same-invocation provenance, native `345B` parity and a complete Up route remain
unverified.
