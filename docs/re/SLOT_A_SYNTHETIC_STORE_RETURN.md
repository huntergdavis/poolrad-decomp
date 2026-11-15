# Synthetic AND store-helper return

The pinned store-word path jumps from `0CD8` to the common epilogue `0E06`,
restores BP and returns with four argument bytes removed. The bounded adapter
checks the caller-supplied immutable jump and epilogue against guest RAM, the
explicit synthetic result and the expected caller frame, then returns to
`3582:0E5A` without writing RAM. Evidence: `U25_STORE_WORD_0C77.md` and
Dunking Sheep memory session `997d7fe9-a031-492f-9b1a-11963b6f8cc5`.

Source byte `95`, earlier predicate and value 11 are synthetic. No same-run
source provenance, native `345B` parity or completed Up route is established.
