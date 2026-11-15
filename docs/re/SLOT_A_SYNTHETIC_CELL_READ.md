# Synthetic AND source read

The unit 25 load-word helper documented in `U25_LOAD_WORD_0E0C.md` reads
`DS:6AB1` at `0F3B`, zero-extends the byte, stores it in the result local and
jumps to `0F4A`. The bounded adapter requires the caller's explicit synthetic
byte to match RAM, authenticates the ten-byte code reference, checks the
normalized index and code/write overlap, and exposes the result and normalized
XOR flags. Mismatches refuse before mutation; caller supplies immutable guest
code and disjoint host objects.

The chain supplies `95` as a synthetic byte. It does not claim the live cell
value or a same-invocation binding. `SLOT_A_FIRST_EXIT_9974.md` identifies an
AND-entry RAM image by SHA-256 `9c82fae60d03939669c8f8a256a96ad0910376c80e07f31ade33eadc1ba04f95`;
a read-only search of 37 capture-sized files in ignored project resources,
excluding build trees, did not locate it. That bounded search does not prove
the image is unavailable elsewhere. The earlier IF predicate and value 11
are synthetic; native `345B` parity and a complete Up route remain unverified.
