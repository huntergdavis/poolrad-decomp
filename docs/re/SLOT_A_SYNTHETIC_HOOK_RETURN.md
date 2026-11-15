# Synthetic class-1 quiet hook return

For the explicit synthetic destination `6E82`, the store hook subtracts
`6B00`, producing selector `0382`. The existing recovered hook has no
matching side-effect branch for that selector and returns to `0CD8` after
normalizing the destination argument. The caller authenticates immutable
hook code; the adapter rejects frame/code overlap. The store-helper return
remains unexecuted.

Evidence: `U25_STORE_WORD_0C77.md`, `ECL_STORE_HOOK_0A74.md` and Dunking
Sheep memory session `3e3ff494-f865-446a-8777-6a4baca8de94`. The source byte
`95`, earlier predicate and value 11 are synthetic. Same-invocation source
provenance, native `345B` parity and a complete Up route remain unverified.
