# AND load-word classifier return

The raw classifier `pr_u25_address_class_06fb_cpu` is documented in
`U25_SLOT_VALUE_0136.md` and `U25_LOAD_WORD_0E0C.md`. The bounded adapter
checks a caller-authenticated 91-byte body, excludes stack/code overlap, and
exposes the classifier's actual status and CPU/RAM state. Caller supplies
immutable guest code and disjoint host objects.

In the synthetic AND chain, the classifier returns class 4 for address
`C04F` at `0E19`; it does not dereference that address. The IF predicate and
earlier value 11 are synthetic. Same-invocation provenance, native `345B`
parity and a complete Up route remain unverified.
