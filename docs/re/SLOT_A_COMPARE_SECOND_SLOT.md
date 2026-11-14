# First COMPARE second numeric operand

The retained script audit in `SLOT_A_UP_ECL_GATE.md` identifies an immediate
14 as the second operand of the first COMPARE. The instruction and call-frame
attribution is in `U1C_COMPARE_0126_CPU.md`. The preceding conditional first
operand boundary is described in `SLOT_A_COMPARE_FIRST_SLOT.md`.

`pr_slot_a_compare_second_slot` stores the first result at SS:BP-2 and
composes the 0189..0190 slot-2 call into the existing raw unit 25 helper. It
exposes the actual helper status and resulting CPU/RAM. It accepts only an
immediate tag and a bounded non-wrapping frame, refusing stack/DS aliases;
the caller authenticates code references and bindings and must ensure guest
writes do not overlap loaded code. The synthetic chain checks the local value
10, returned literal 14, the earlier SAVE effect and atomic unsupported-tag
refusal. It does not establish same-invocation provenance, the full Up path,
or native 345B parity.
