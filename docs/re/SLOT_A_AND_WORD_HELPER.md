# Synthetic AND destination word helper

`U1C_AND_OR_0DEF.md` documents the destination word-helper call at `0E1B`.
The bounded adapter reuses the raw U25 `06AF` composition, authenticates the
caller-supplied call, stub and helper references, and requires the saved
second result to match an explicit synthetic byte and the arguments to encode
`6E82`. It exposes actual helper status and CPU/RAM, including altered returns.
Invalid bindings and unsupported input refuse before mutation. Caller supplies
immutable guest code, disjoint host objects and keeps guest writes away from
loaded code.

The synthetic chain assembles `6E82` and returns to `0E20`, retaining the
synthetic second value `95`. The byte is not a measured live `DS:6AB1` value;
the earlier IF predicate and value 11 are synthetic. Same-invocation
provenance, native `345B` parity and a complete Up route remain unverified.
