# Synthetic AND handler return

The recovered AND handler epilogue `0E5A..0E5D` restores BP and returns
to the dispatcher. This bounded adapter authenticates the supplied immutable
tail, synthetic result and expected return frame, then stops at `3582:30F7`.
It makes no RAM writes. Evidence: `U1C_AND_OR_0DEF.md` and Dunking Sheep
memory session `01a13101-7ee2-70c1-914b-421afef5e33a`. Source byte `95`,
earlier predicate and value 11 are synthetic; no same-run source provenance,
native `345B` parity or complete Up route is established.
