# Synthetic AND dispatcher return

The AND dispatch return at `30F7` jumps to the common `316F` epilogue,
which restores BP and returns to the runner. The bounded adapter checks the
caller-supplied immutable jump and tail, synthetic result and expected runner
frame, then returns to `3582:31A8` with RAM unchanged. The chain test installs
the synthetic common-tail fixture at this boundary; it is not a same-run capture. Evidence: `SLOT_A_AND_DISPATCH.md`,
`U1C_AND_OR_0DEF.md`, and Dunking Sheep memory session
`3e3ff494-f865-446a-8777-6a4baca8de94`.

Source byte `95`, earlier predicate and value 11 are synthetic; no same-run
source provenance, native `345B` parity or complete Up route is established.
